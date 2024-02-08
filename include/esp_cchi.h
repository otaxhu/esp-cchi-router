/**
 * Chi-like router for the ESP-IDF esp_http_server component 
 * 
 * The purpose of this library is to provide a pattern matching like the Golang's Chi router does
 * with the URIs
 * 
 * Quick Usage:
 * 
 * static esp_err_t handle_ping(httpd_req_t *r) {
 * 
 *     // esp_cchi_get_uri_param_len is the function for extracting the URI param length from the
 *     // request, this will return the param_len, not counting the NUL character that ends it
 *     size_t param_len = esp_cchi_get_uri_param_len(r, "my_param");
 *     if (param_len == 0) {
 *         return ESP_FAIL;
 *     }
 *     char param_content[param_len + 1];
 *     size_t bytes_written = 0;
 * 
 *     if (esp_cchi_get_uri_param(r, "my_param", param_content, param_len, &bytes_written) != ESP_OK) {
 *         return ESP_FAIL;
 *     }
 * 
 *     // Most likely bytes_written and param_len will be the same value if you use the
 *     // esp_cchi_get_uri_param_len function, but if you don't, you will prefer using the
 *     // bytes_written value as a best practice to find the end of the string and add the NUL char
 *     // yourself
 *     param_content[bytes_written] = '\0';
 * 
 *     printf("my_param = %s\n", param_content);
 * 
 *     return httpd_resp_send(r, param_content, bytes_written);
 * }
 * 
 * static httpd_uri_t ping_uri = {
 *     .uri = "/{my_param}",
 *     .method = HTTPD_GET,
 *     .handler = handle_ping
 * };
 * 
 * static const char *TAG = "app";
 * 
 * void app_main() {
 *     httpd_config_t hd_config = HTTPD_DEFAULT_CONFIG();
 *     esp_err_t err = esp_cchi_setup_hd_config(&hd_config);
 *     if (err != ESP_OK) {
 *         ESP_LOGE(TAG, "error ocurred configuring hd_config");
 *         return;
 *     }
 *     if ((err = esp_cchi_setup_uri(&ping_uri)) != ESP_OK) {
 *         ESP_LOGE(TAG, "error ocurred configuring ping_uri");
 *         return;
 *     }
 *     httpd_handle_t server = NULL;
 * 
 *     // You must've been called esp_cchi_setup_uri on all of your uri structs before calling
 *     // httpd_start and httpd_register_uri_handler
 *     httpd_start(&server, &hd_config);
 *     httpd_register_uri_handler(server, &ping_uri);
 * 
 *     while (1) {
 *         // Some event that triggers the shutdown of the server
 *         if (!event) {
 *             continue;
 *         }
 *         httpd_stop(server);
 *         // You must use this function to free the memory allocated in the user_ctx uri's member
 *         // struct. It's important to free it after calling httpd_stop, the reason is because you
 *         // would free memory that is in use in your uri handlers.
 *         esp_cchi_delete_hd_uri(&ping_uri, true);
 *         return;
 *     }
 * }
*/
#ifndef __ESP_CCHI_ROUTER_HEADER
#define __ESP_CCHI_ROUTER_HEADER

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
 *  - ESP_ERR_INVALID_ARG if "hd_cfg" is NULL
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
 *  - ESP_ERR_INVALID_ARG if "uri" is NULL
*/
esp_err_t esp_cchi_setup_hd_uri(httpd_uri_t *hd_uri);

/**
 * @param hd_uri Pointer to httpd_uri_t, must not be NULL
 * @param no_dangling_ctx Boolean value that indicates if the .user_ctx struct member will be set
 * to NULL or not. "true" sets .user_ctx to NULL, "false" doesn't
 * 
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARG if hd_uri is NULL
*/
esp_err_t esp_cchi_delete_hd_uri(httpd_uri_t *hd_uri, bool no_dangling_ctx);

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
 *  - ESP_ERR_INVALID_ARG if any of the arguments are NULL or are invalid
*/
esp_err_t esp_cchi_get_uri_param(httpd_req_t *r,
                                 const char *uri_param,
                                 char *buf,
                                 size_t buf_len,
                                 size_t *bytes_written);

/**
 * @param r Pointer to httpd_req_t, must be a request correctly initialized during the
 * esp_cchi_setup_uri function call step
 * @param uri_param Pointer to a string, must contain the uri_param that you are searching for
 * 
 * @returns
 *  - 0: if r or uri_param are invalid or if the uri_param doesn't exists
 *  - Number of bytes that the uri_param content is long
*/
size_t esp_cchi_get_uri_param_len(httpd_req_t *r, const char *uri_param);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_CCHI_ROUTER_HEADER */
