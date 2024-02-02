#include <esp_http_server.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool esp_cchi_uri_match_fn(const char *ref_uri, const char *uri, size_t match_upto) {

}

esp_err_t esp_cchi_setup_hd_config(httpd_config_t *hd_cfg) {
    if (hd_cfg == NULL) {
        return ESP_ERR_INVALID_ARGUMENT;
    }
    hd_cfg->uri_match_fn = esp_cchi_uri_match_fn;
    return ESP_OK;
}

static struct key_value {
    char *key, *value;
};

static esp_err_t parse_uri_params(const char *uri, size_t uri_len, struct key_value **array_kv) {
    if (uri == NULL || uri_len == 0) {
        return ESP_ERR_INVALID_ARGUMENT;
    }

    size_t array_kv_len = 8;

    size_t kv_assigned = 0;

    size_t key_len = 100;
    size_t value_len = 1024;

    char buf[100];

main_loop:
    for (size_t i = 0; i < uri_len; i++) {
        if (uri[i] != '{') {
            continue;
        }

        i++;

        if (i >= uri_len) {
            break;
        }

        if (uri[i] == '}' || uri[i] == ':') {
            continue;
        }

        if (array_kv == NULL) {
            return ESP_ERR_INVALID_ARGUMENT;
        }

        if (*array_kv == NULL) {
            *array_kv = malloc(sizeof(struct key_value) * array_kv_len);
            if (*array_kv == NULL) {
                return ESP_ERR_NO_MEM;
            }
        }

        size_t bytes_written_to_buf = 0;

        for (size_t j = 0; j < sizeof(buf); j++) {
            if (i >= uri_len) {
                break main_loop;
            }
            if (uri[i] == '}' || uri[i] == ':') {
                break;
            }

            buf[j] = uri[i];
            bytes_written_to_buf++;
        }

        
    }
}

esp_err_t esp_cchi_setup_uri(httpd_uri_t *uri) {
    if (uri == NULL || uri->uri == NULL) {
        return ESP_ERR_INVALID_ARGUMENT;
    }

    uri->user_ctx = 
}
