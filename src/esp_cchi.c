#include <esp_http_server.h>
#include <esp_cchi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define __ESP_CCHI_CTX_MAGIC_SIZE 19
static const char *esp_cchi_ctx_magic = "ESP_CCHI_CTX_MAGIC";

static struct esp_cchi_ctx {
    char magic[__ESP_CCHI_CTX_MAGIC_SIZE];
    const char *ref_uri;
};

static bool esp_cchi_uri_match_fn(const char *ref_uri, const char *uri, size_t match_upto) {
    const char *traverse_ref_uri = ref_uri;
    const char *traverse_uri = uri;

    while (((*traverse_ref_uri) != '\0') && ((*traverse_uri) != '\0')) {
        if ((*traverse_ref_uri) != '{') {
            if ((*traverse_ref_uri) != (*traverse_uri)) {
                return false;
            }
            traverse_ref_uri++;
            traverse_uri++;
            continue;
        }
        traverse_ref_uri = strchr(traverse_ref_uri, '}');
        if (traverse_ref_uri == NULL) {
            return false;
        }
        traverse_ref_uri++;
        if ((*traverse_ref_uri) == '{') {
            // Here we return false because of the ambiguity of the placeholder,
            // The router will consider this an error
            return false;
        }

        const char *slash_pos = strchr(traverse_uri, '/');
        const char *temp_traverse_uri = strchr(traverse_uri, *traverse_ref_uri);

        if (temp_traverse_uri == NULL) {
            return false;
        }

        if ((temp_traverse_uri - traverse_uri) < 1) {
            return false;
        }

        traverse_uri = temp_traverse_uri;

        if (slash_pos != NULL && traverse_uri > slash_pos) {
            return false;
        }
    }

    if (((*traverse_uri) != '\0') || ((*traverse_ref_uri) != '\0')) {
        return false;
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

// TODO: Validation of .uri member, validate that the uri provided is a valid uri
esp_err_t esp_cchi_setup_hd_uri(httpd_uri_t *hd_uri) {
    if (hd_uri == NULL || hd_uri->uri == NULL) {
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
