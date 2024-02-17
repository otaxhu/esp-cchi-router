#include <esp_http_server.h>
#include <esp_cchi/router.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define __ESP_CCHI_CTX_MAGIC_SIZE 19
static const char *const esp_cchi_ctx_magic = "ESP_CCHI_CTX_MAGIC";

struct esp_cchi_ctx {
    char magic[__ESP_CCHI_CTX_MAGIC_SIZE];
    const char *ref_uri;
};

static bool esp_cchi_uri_match_fn(const char *ref_uri, const char *uri, size_t match_upto) {
    while (((*ref_uri) != '\0') && ((*uri) != '\0')) {
        if ((*ref_uri) != '{') {
            if ((*ref_uri) != (*uri)) {
                return false;
            }
            ref_uri++;
            uri++;
            continue;
        }
        ref_uri = strchr(ref_uri, '}');
        ref_uri++;

        const char *slash_pos = strchr(uri, '/');
        const char *temp_uri = strchr(uri, *ref_uri);

        const char *last_char = temp_uri;

        if ((*ref_uri) != '\0') {
            while (1) {
                temp_uri = strchr(temp_uri + 1, *ref_uri);
                if (temp_uri == NULL) {
                    break;
                }
                if (slash_pos != NULL && slash_pos < temp_uri) {
                    break;
                }
                last_char = temp_uri;
            }
        }

        if (last_char == NULL) {
            return false;
        }

        // TODO: Not quite sure if this line does what I want it to do :/
        if ((last_char - uri) < 1) {
            return false;
        }

        uri = last_char;

        if (slash_pos != NULL && uri > slash_pos) {
            return false;
        }
    }

    if (((*uri) != '\0') || ((*ref_uri) != '\0')) {
        return false;
    }

    return true;
}

static bool esp_cchi_is_valid_uri(const char *uri) {
    if ((*uri) != '/') {
        return false;
    }
    uri++;
    while ((*uri) != '\0') {
        if ((*uri) != '{') {
            uri++;
            continue;
        }
        const char *param_end = strchr(uri, '}');
        if (param_end == NULL) {
            return false;
        }
        uri = param_end + 1;
        if ((*uri) == '{') {
            return false;
        }
    }
    return true;
}

esp_err_t esp_cchi_setup_hd_config(httpd_config_t *hd_cfg) {
    if (hd_cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    hd_cfg->uri_match_fn = esp_cchi_uri_match_fn;
    return ESP_OK;
}

esp_err_t esp_cchi_setup_hd_uri(httpd_uri_t *hd_uri) {
    if (hd_uri == NULL || hd_uri->uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!esp_cchi_is_valid_uri(hd_uri->uri)) {
        return ESP_ERR_INVALID_ARG;
    }

    struct esp_cchi_ctx *ctx = malloc(sizeof(struct esp_cchi_ctx));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    strcpy(ctx->magic, esp_cchi_ctx_magic);
    ctx->ref_uri = hd_uri->uri;

    hd_uri->user_ctx = ctx;

    return ESP_OK;
}

esp_err_t esp_cchi_delete_hd_uri(httpd_uri_t *hd_uri, bool no_dangling_ctx) {
    if (hd_uri == NULL || hd_uri->user_ctx) {
        return ESP_ERR_INVALID_ARG;
    }

    struct esp_cchi_ctx *ctx = (struct esp_cchi_ctx*)hd_uri->user_ctx;
    if (strcmp(ctx->magic, esp_cchi_ctx_magic) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    free(ctx);

    if (no_dangling_ctx) {
        hd_uri->user_ctx = NULL;
    }

    return ESP_OK;
}

esp_err_t esp_cchi_get_uri_param(httpd_req_t *r,
                                 const char *uri_param,
                                 char *buf,
                                 size_t buf_len,
                                 size_t *bytes_written)
{
    if (r == NULL || r->user_ctx == NULL || uri_param == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    struct esp_cchi_ctx *ctx = (struct esp_cchi_ctx*)r->user_ctx;

    if (strcmp(ctx->magic, esp_cchi_ctx_magic) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t uri_param_len = strlen(uri_param);

    const char *traverse_ref_uri = ctx->ref_uri;
    const char *traverse_uri = r->uri;

    while (((*traverse_ref_uri) != '\0') && ((*traverse_uri) != '\0')) {
        if ((*traverse_ref_uri) != '{') {
            traverse_ref_uri++;
            traverse_uri++;
            continue;
        }
        const char *param_end = strchr(traverse_ref_uri, '}');
        size_t ref_param_len = param_end - traverse_ref_uri - 1;
        const char *colon_pos = strchr(traverse_ref_uri, ':');
        if (colon_pos != NULL && colon_pos < param_end) {
            ref_param_len = colon_pos - traverse_ref_uri - 1;
        }
        if (uri_param_len != ref_param_len) {
            traverse_ref_uri = param_end + 1;
            traverse_uri = strchr(traverse_uri, *(param_end + 1));
            continue;
        }
        if (strncmp(traverse_ref_uri + 1, uri_param, ref_param_len) != 0) {
            traverse_ref_uri = param_end + 1;
            traverse_uri = strchr(traverse_uri, *(param_end + 1));
            continue;
        }
        const char *param_content_end = strchr(traverse_uri, *(param_end + 1));
        size_t param_content_len = param_content_end - traverse_uri;
        if (param_content_len > buf_len) {
            *bytes_written = 0;
            return ESP_FAIL;
        }
        strncpy(buf, traverse_uri, param_content_len);
        *bytes_written = param_content_len;
        return ESP_OK;
    }
    *bytes_written = 0;
    return ESP_ERR_NOT_FOUND;
}

size_t esp_cchi_get_uri_param_len(httpd_req_t *r, const char *uri_param) {
    if (r == NULL || r->user_ctx == NULL || uri_param == NULL) {
        return 0;
    }
    struct esp_cchi_ctx *ctx = (struct esp_cchi_ctx*)r->user_ctx;
    if (strcmp(ctx->magic, esp_cchi_ctx_magic) != 0) {
        return 0;
    }

    size_t uri_param_len = strlen(uri_param);

    const char *traverse_ref_uri = ctx->ref_uri;
    const char *traverse_uri = r->uri;

    while ((*traverse_ref_uri) != '\0' && (*traverse_uri) != '\0') {
        if ((*traverse_ref_uri) != '{') {
            traverse_ref_uri++;
            traverse_uri++;
            continue;
        }
        const char *param_end = strchr(traverse_ref_uri, '}');
        size_t ref_param_len = param_end - traverse_ref_uri - 1;
        const char *colon_pos = strchr(traverse_ref_uri, ':');
        if (colon_pos != NULL && colon_pos < param_end) {
            ref_param_len = colon_pos - traverse_ref_uri - 1;
        }
        if (uri_param_len != ref_param_len) {
            traverse_ref_uri = param_end + 1;
            traverse_uri = strchr(traverse_uri, *(param_end + 1));
            continue;
        }
        if (strncmp(traverse_ref_uri + 1, uri_param, ref_param_len) != 0) {
            traverse_ref_uri = param_end + 1;
            traverse_uri = strchr(traverse_uri, *(param_end + 1));
            continue;
        }
        const char *param_content_end = strchr(traverse_uri, *(param_end + 1));
        return param_content_end - traverse_uri;
    }

    // Param not found
    return 0;
}
