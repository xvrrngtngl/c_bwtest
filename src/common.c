#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "common.h"

size_t write_cb(char *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr)
        return 0;

    mem->memory = ptr;
    memcpy(&mem->memory[mem->size], contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

size_t discard_cb(char *ptr, size_t size, size_t nmemb, void *userp)
{
    (void)ptr;
    (void)userp;

    return size * nmemb;
}

curl_off_t easy_http_get(const char *url, long timeout_ms, struct MemoryStruct *response)
{
    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    if (response) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    }

    CURLcode result = curl_easy_perform(curl);

    curl_off_t bytes_per_sec = 0;
    curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &bytes_per_sec);

    if (result == CURLE_OPERATION_TIMEDOUT && bytes_per_sec > 0)
        result = CURLE_OK;

    curl_easy_cleanup(curl);

    return result == CURLE_OK ? bytes_per_sec : -(curl_off_t)result;
}

curl_off_t easy_http_post(const char *url, const char *payload, long payload_size, long timeout_ms, struct MemoryStruct *response)
{
    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)payload_size);

    if (response) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    }

    CURLcode result = curl_easy_perform(curl);

    curl_off_t bytes_per_sec = 0;
    curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD_T, &bytes_per_sec);

    if (result == CURLE_OPERATION_TIMEDOUT && bytes_per_sec > 0)
        result = CURLE_OK;

    curl_easy_cleanup(curl);

    return result == CURLE_OK ? bytes_per_sec : -(curl_off_t)result;
}

int responds(const char *host, long timeout_ms)
{
    char url[256];
    snprintf(url, sizeof url, "http://%s", host);

    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);

    CURLcode result = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    return result == CURLE_OK;
}

char *read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    file = fopen(filename, "rb");
    if (file == NULL)
        goto cleanup;

    if (fseek(file, 0, SEEK_END) != 0)
        goto cleanup;
    length = ftell(file);
    if (length < 0)
        goto cleanup;
    if (fseek(file, 0, SEEK_SET) != 0)
        goto cleanup;

    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL)
        goto cleanup;

    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL)
        fclose(file);

    return content;
}
