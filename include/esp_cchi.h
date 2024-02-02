/**
 * Chi-like router for the ESP-IDF esp_http_server component 
 * 
 * The purpose of this library is to provide a pattern matching like the Golang's Chi router does
 * with the URIs
 * 
 * Quick Usage:
 * 
 * esp_err_t handle_ping(httpd_req_t *r) {
 * 
 *     // esp_cchi_get_uri_param is the function for extracting the URI param from the URI
 * 
 *     size_t bytes_written = 0;
 *     char buf[100];
 * 
 *     size_t bytes_written_dyn = 0;
 *     size_t len_dyn = sizeof(char) * 1024;
 *     char *buf_dyn = malloc(len_dyn);
 * 
 *     do {
 *         if (esp_cchi_get_uri_param(r, "my_param", buf, sizeof(buf), &bytes_written) != ESP_OK) {
 *             return ESP_FAIL;
 *         }
 *         for (size_t i = 0; i < bytes_written; i++) {
 *             if ((bytes_written_dyn + bytes_written) > len_dyn) {
 *                 len_dyn += 1024;
 *                 buf_dyn = realloc(buf_dyn, len_dyn);
 *             }
 *             buf_dyn[bytes_written_dyn] = buf[i];
 *             bytes_written_dyn++;
 *         }
 *     } while (bytes_written > 0);
 * 
 *     // esp_cchi_get_uri_param doesn't null terminate the string, so you gonna have to do it
 *     // yourself
 *     if ((bytes_written_dyn + 1) > (len_dyn - 1)) {
 *         buf_dyn = realloc(buf_dyn, len_dyn + 1);
 *     }
 *     buf_dyn[bytes_written_dyn + 1] = '\0';
 *     printf("\"my param\" = %s\n", buf_dyn);
 *     return httpd_resp_send(r, buf_dyn, bytes_written_dyn);
 * }
 * 
 * static httpd_uri_t ping_uri = {
 *     .uri = "/{my_param}",
 *     .method = HTTPD_GET,
 *     .handler = handle_ping
 * };
 * 
 * void app_main() {
 *     httpd_config_t hd_config = HTTPD_DEFAULT_CONFIG();
 *     hd_config.uri_match_fn = esp_cchi_uri_match_fn;
 *     esp_cchi_init_uri(&ping_uri);
 *     httpd_handle_t server = NULL;
 *     httpd_start(&server, &hd_config);
 *     httpd_register_uri_handler(server, &ping_uri);
 * }
*/
#ifndef __ESP_CCHI_ROUTER_HEADER
#define __ESP_CCHI_ROUTER_HEADER

#include <stdio.h>
#include <esp_http_server.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function that sets the .uri_match_fn member of hd_cfg to a matcher that matches the URI like
 * this:
 *
 * "/path/{<uri_param>:<regexp>}_more_path"
 *
 * The curly braces means that everything inside of that pattern is going to be named as
 * "uri_param" name and can be retrieved with esp_cchi_get_uri_param function, optionally a colon
 * can be placed, this will mean that everything after that will be a regular expression that
 * "uri_param" is going to match
 *
 * If no regular expression is specified, this will mean that is going to match anything except for
 * empty string
 *
 * The URI param will match until next characters matches or if there is no more characters, will
 * match until next forward slash, as an example "/{my_param}-foo"; the URIs that matches with this
 * pattern could be "/hello-foo", "/hello-world-foo", etc. but this will not match "/-foo",
 * "/hello-foo/bar", etc.
 *
 * @param hd_cfg Pointer to a httpd_config_t, must not be NULL
 *
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARGUMENT if "hd_cfg" is NULL
*/
esp_err_t esp_cchi_setup_hd_config(httpd_config_t *hd_cfg);

/**
 * Function that sets the .user_ctx member of uri to a data structure that holds all of the URI
 * params
 *
 * @param uri Pointer to a httpd_uri_t, must not be NULL
 *
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARGUMENT if "uri" is NULL
*/
esp_err_t esp_cchi_setup_uri(httpd_uri_t *uri);

/**
 * @param[in] r Pointer to httpd_req_t struct
 * @param[in] uri_param Pointer to null terminated string that contains the string of the URI param
 * that is gonna be retrieved
 * @param[out] buf Pointer to buffer, in this buffer is going to be written the URI param value
 * @param[in] buf_len Length of the buffer
 * @param[out] bytes_written Pointer to size_t type variable, in this pointer is going to be
 * written how many bytes were written to "buf"
 * 
 * @returns
 *  - ESP_OK on success (even if there is no more bytes to write to buffer)
 *  - ESP_ERR_INVALID_ARGUMENT if any of the arguments are NULL or are invalid
*/
esp_err_t esp_cchi_get_uri_param(httpd_req_t *r, const char *uri_param, char *buf, size_t buf_len, size_t *bytes_written);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_CCHI_ROUTER_HEADER */
