#pragma once

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t middlewares_logger(httpd_req_t *r);

#define middlewares_allow_content_types(middleware_fn_name, ...)                                 \
esp_err_t middleware_fn_name(httpd_req_t *r) {                                                   \
    char* content_types[] = {__VA_ARGS__};                                                       \
    size_t content_types_len = sizeof(content_types) / sizeof(char *);                           \
    if (content_types_len == 0) {                                                                \
        return ESP_OK;                                                                           \
    }                                                                                            \
    char *status;                                                                                \
    esp_err_t err = ESP_OK;                                                                      \
    size_t req_ct_len = httpd_req_get_hdr_value_len(r, "Content-Type");                          \
    if (req_ct_len == 0) {                                                                       \
        status = HTTPD_400;                                                                      \
        err = ESP_FAIL;                                                                          \
        goto exit;                                                                               \
    }                                                                                            \
    char* ct_buf = malloc(req_ct_len + 1);                                                       \
    if (ct_buf == NULL) {                                                                        \
        status = HTTPD_500;                                                                      \
        err = ESP_ERR_NO_MEM;                                                                    \
        goto exit;                                                                               \
    }                                                                                            \
    err = httpd_req_get_hdr_value_str(r, "Content-Type", ct_buf, req_ct_len + 1);                \
    if (err != ESP_OK) {                                                                         \
        status = HTTPD_500;                                                                      \
        err = ESP_FAIL;                                                                          \
        goto exit;                                                                               \
    }                                                                                            \
    for (size_t i = 0; i < content_types_len; i++) {                                             \
        size_t allow_ct_len = strlen(content_types[i]);                                          \
        if (allow_ct_len != req_ct_len) {                                                        \
            continue;                                                                            \
        }                                                                                        \
        if (strncmp(ct_buf, content_types[i], req_ct_len) == 0) {                                \
            goto exit;                                                                           \
        }                                                                                        \
    }                                                                                            \
    /* Content type not found in allowed content types array, setting status to 415 */           \
    err = ESP_FAIL;                                                                              \
    status = "415 Unsupported Media Type";                                                       \
exit:                                                                                            \
    if (err != ESP_OK) {                                                                         \
        httpd_resp_set_status(r, status);                                                        \
        httpd_resp_send(r, NULL, 0);                                                             \
    }                                                                                            \
    free(ct_buf);                                                                                \
    return err;                                                                                  \
}

#ifdef __cplusplus
}
#endif
