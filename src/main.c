#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <curl/curl.h>

#include "cjson/cJSON.h"
#include "common.h"

#define QUICK_PROBE_TIMEOUT_MS (1L * 1000)
#define LOOKUP_LOCATION_TIMEOUT_MS (2L * 1000)
#define TEST_TIMEOUT_MS (15L * 1000)

#define TEST_DOWNLOAD_PAYLOAD_SIZE (25 * 1000 * 1000)
#define TEST_UPLOAD_PAYLOAD_SIZE (25 * 1000 * 1000)

#define DATA_DIR "./data/"
#define HOST_LIST_PATH DATA_DIR "pretty_host_list.json"
#define LOOKUP_LOCATION_EP "http://ip-api.com/json/?fields=status,country"

/**
 * Look up location (country) of current public IP via external API
 *
 * @param location Buffer to receive the location (country) string
 * @param size Size of @p location buffer in bytes
 */
static void lookup_location(char *location, size_t size)
{
    /* remains Unknown while pending */
    snprintf(location, size, "Unknown");

    struct MemoryStruct body = {0};
    if (easy_http_get(LOOKUP_LOCATION_EP, LOOKUP_LOCATION_TIMEOUT_MS, &body) >= 0) {
        cJSON *json = cJSON_Parse(body.memory);
        cJSON *field = cJSON_GetObjectItemCaseSensitive(json, "country");
        if (cJSON_IsString(field) && field->valuestring)
            snprintf(location, size, "%s", field->valuestring);
        cJSON_Delete(json);
    }

    free(body.memory);
}

/**
 * Measure download speed of server specified
 *
 * @param host Address of the server with port e.g. airconect.net:8080
 * @param payload_bytes Payload size in bytes
 *
 * @return Download speed in bytes/s or CURLcode on failure
 */
static curl_off_t measure_download_speed(const char *host, int payload_bytes)
{
    char url[256];
    snprintf(url, sizeof url, "http://%s/download?size=%d", host, payload_bytes);

    return easy_http_get(url, TEST_TIMEOUT_MS, NULL);
}

/**
 * Measure upload speed of server specified
 *
 * @param host Address of the server with port e.g. airconect.net:8080
 * @param payload_bytes Payload size in bytes
 *
 * @return Upload speed in bytes/s or CURLcode on failure
 */
static curl_off_t measure_upload_speed(const char *host, int payload_bytes)
{
    char url[256];
    snprintf(url, sizeof url, "http://%s/upload", host);

    char *payload = calloc(payload_bytes, 1);
    curl_off_t bytes_per_sec = easy_http_post(url, payload, payload_bytes, TEST_TIMEOUT_MS, NULL);
    free(payload);

    return bytes_per_sec;
}

/**
 * Find first responding server in location specified
 *
 * @param location Location to lookup in HOST_LIST_PATH e.g. Brazil
 * @param host Buffer to receive server address string
 * @param size Size of @p host buffer in bytes
 */
static void find_best_server(const char *location, char *host, size_t size)
{
    host[0] = '\0';

    char *text = read_file(HOST_LIST_PATH);
    cJSON *servers = cJSON_Parse(text);
    cJSON *hosts = cJSON_GetObjectItemCaseSensitive(servers, location);

    cJSON *h;
    cJSON_ArrayForEach(h, hosts) {
        if (cJSON_IsString(h) && responds(h->valuestring, QUICK_PROBE_TIMEOUT_MS)) {
            snprintf(host, size, "%s", h->valuestring);
            break;
        }
    }

    cJSON_Delete(servers);
    free(text);
}

static void print_speed(const char *label, curl_off_t speed)
{
    if (speed >= 0)
        printf("%s %.2f Mbit/s\n", label, speed * 8.0 / (1000 * 1000));
    else
        printf("%s failed: %s\n", label, curl_easy_strerror((CURLcode)-speed));
}

/**
 * Perform the full test sequence
 * lookup_location, find_best_server, measure_download_speed, measure_upload_speed
 *
 * @return 0 on success 1 on failure
 */
static int full_test(void)
{
    char location[64];
    fprintf(stderr, "Detecting location...\n");
    lookup_location(location, sizeof location);

    char host[256];
    fprintf(stderr, "Finding best server...\n");
    find_best_server(location, host, sizeof host);
    if (!host[0]) {
        fprintf(stderr, "No server found for %s\n", location);
        return 1;
    }

    fprintf(stderr, "Download speed of %s...\n", host);
    curl_off_t down = measure_download_speed(host, TEST_DOWNLOAD_PAYLOAD_SIZE);
    fprintf(stderr, "Upload speed of %s...\n", host);
    curl_off_t up = measure_upload_speed(host, TEST_UPLOAD_PAYLOAD_SIZE);

    printf("\nLocation – %s\n", location);
    printf("Server – %s\n", host);
    print_speed("Download –", down);
    print_speed("Upload –", up);

    return (down < 0 || up < 0) ? 1 : 0;
}

int main(int argc, char *argv[])
{
    curl_global_init(CURL_GLOBAL_ALL);

    char location[64];
    char host[256];

    int opt;
    while ((opt = getopt(argc, argv, "ls:d:u:")) != -1) {
        switch (opt) {
            case 'l':
                lookup_location(location, sizeof location);
                printf("Location – %s\n", location);
                break;
            case 's':
                find_best_server(optarg, host, sizeof host);
                if (host[0])
                    printf("Server – %s\n", host);
                else
                    printf("No host for location %s\n", optarg);
                break;
            case 'd':
                print_speed("Download –", measure_download_speed(optarg, TEST_DOWNLOAD_PAYLOAD_SIZE));
                break;
            case 'u':
                print_speed("Upload –", measure_upload_speed(optarg, TEST_UPLOAD_PAYLOAD_SIZE));
                break;
        }
    }

    int res = (optind == 1) ? full_test() : 0;

    curl_global_cleanup();
    return res;
}
