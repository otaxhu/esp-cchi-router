/**
 * ============= Middleware API ===============
 * Functions and types that has the prefix "esp_cchi_mw_" are part of the Middleware API
*/
#ifndef __ESP_CCHI_MIDDLEWARE_HEADER
#define __ESP_CCHI_MIDDLEWARE_HEADER

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type that represents a middleware group or middleware chain
*/
typedef struct esp_cchi_mw_group {
    esp_err_t (**__mw_array)(httpd_req_t *);
    size_t __mw_array_len;
} esp_cchi_mw_group_t;

/**
 * @param[out] mw_group Pointer to middleware group type, must not be NULL
 * 
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARG if "mw_group" is NULL
*/
esp_err_t esp_cchi_mw_create_group(esp_cchi_mw_group_t *mw_group);

/**
 * @param mw_group Pointer to middleware group type, must not be NULL
 * 
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARG if "mw_group" is NULL
*/
esp_err_t esp_cchi_mw_delete_group(esp_cchi_mw_group_t *mw_group);

/**
 * @param mw_group Pointer to middleware group type, must not be NULL
 * @param middleware Pointer to middleware handler function, must not be NULL, has the same type as
 *        .handler struct member of httpd_uri_t type
 * 
 * @returns
 *  - ESP_OK on success
 *  - ESP_ERR_INVALID_ARG if some of the arguments are NULL
 *  - ESP_ERR_NO_MEM if there is no memory available to allocate the middleware (the middleware
 *    group will not be modified nor freed up)
*/
esp_err_t esp_cchi_mw_use(esp_cchi_mw_group_t *mw_group, esp_err_t (*middleware)(httpd_req_t *));

/**
 * @param handler_fn_name Unique name which you gonna call the function that is going to be created
 * @param mw_group Pointer to scoped middleware group type
 * @param final_handler Pointer to function handler that is going to act as the final handler of
 *        the middleware chain
 * 
 * @returns Function that must be in global scope
 * 
 * @note if you want the generated function to be static, you need to add "static" just before
 *       calling this macro
*/
#define esp_cchi_mw_build(handler_fn_name, mw_group, final_handler)                              \
esp_err_t handler_fn_name(httpd_req_t *r) {                                                      \
    esp_err_t err;                                                                               \
    for (size_t i = 0; i < (mw_group)->__mw_array_len; i++) {                                    \
        if (err = (mw_group)->__mw_array[i](r)) {                                                \
            return err;                                                                          \
        }                                                                                        \
    }                                                                                            \
    return (final_handler)(r);                                                                   \
}

#ifdef __cplusplus
}
#endif
#endif /* __ESP_CCHI_MIDDLEWARE_HEADER */
