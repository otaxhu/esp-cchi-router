#include <esp_http_server.h>
#include <esp_cchi/middleware.h>

esp_err_t esp_cchi_mw_create_group(esp_cchi_mw_group_t *mw_group) {
    if (mw_group == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *mw_group = (esp_cchi_mw_group_t){ 0 };
    return ESP_OK;
}

esp_err_t esp_cchi_mw_delete_group(esp_cchi_mw_group_t *mw_group) {
    if (mw_group == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    free(mw_group->__mw_array);
    mw_group->__mw_array = NULL;
    mw_group->__mw_array_len = 0;
    return ESP_OK;
}

esp_err_t esp_cchi_mw_use(esp_cchi_mw_group_t *mw_group, esp_err_t (*middleware)(httpd_req_t *)) {
    if (mw_group == NULL || middleware == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t (**temp_ptr)(httpd_req_t *) = realloc(mw_group->__mw_array,
                                                    sizeof(temp_ptr) * (mw_group->__mw_array_len + 1));
    if (temp_ptr == NULL) {
        return ESP_ERR_NO_MEM;
    }
    mw_group->__mw_array = temp_ptr;
    mw_group->__mw_array[mw_group->__mw_array_len] = middleware;
    mw_group->__mw_array_len++;
    return ESP_OK;
}
