idf_component_register(SRCS "src/esp_cchi_router.c"
                            "src/esp_cchi_mw.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server)
