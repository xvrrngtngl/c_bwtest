#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

/**
 * libcurl write callback
 *
 * @see https://curl.se/libcurl/c/getinmemory.html
 *
 * @return Number of bytes handled or 0 on allocation failure
 */
size_t write_cb(char *contents, size_t size, size_t nmemb, void *userp);

/* write_cb counterpart to discard instead of keeping */
size_t discard_cb(char *ptr, size_t size, size_t nmemb, void *userp);

/**
 * HTTP GET url and measure download speed
 *
 * @see https://curl.se/libcurl/c/CURLINFO_SPEED_DOWNLOAD_T.html
 *
 * @param url URL to HTTP GET
 * @param timeout_ms Timeout in MS
 * @param response Buffer to capture response or NULL to discard
 *
 * @return Measured download speed in bytes/s or CURLcode on failure
 */
curl_off_t easy_http_get(const char *url, long timeout_ms, struct MemoryStruct *response);

/**
 * HTTP POST payload to url and measure upload speed
 *
 * @see https://curl.se/libcurl/c/http-post.html
 *
 * @param url URL to HTTP POST
 * @param payload Payload to POST
 * @param payload_size Payload size in bytes
 * @param timeout_ms Timeout in MS
 * @param response Buffer to capture response or NULL to discard
 *
 * @return Measured upload speed in bytes/s or CURLcode on failure
 */
curl_off_t easy_http_post(const char *url, const char *payload, long payload_size, long timeout_ms, struct MemoryStruct *response);

/**
 * Quick TCP reachability probe
 *
 * @see https://curl.se/libcurl/c/CURLOPT_CONNECT_ONLY.html
 *
 * @param host Address of the server e.g. speedtest.litnet.lt:8080
 * @param timeout_ms Timeout in MS
 *
 * @return If host accepted connection (1) or not (0)
 */
int responds(const char *host, long timeout_ms);

/**
 * Load entire file into dynamically allocated memory buffer
 *
 * @see https://github.com/DaveGamble/cJSON/blob/v1.7.19/tests/common.h
 *
 * @param filename Path to file to read
 *
 * @return Buffer with file content or NULL on failure
 *
 * @note Caller must free()
 */
char *read_file(const char *filename);

#endif /* COMMON_H */
