idf_component_register(SRCS "mqtt_cli.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS ""
                    REQUIRES esp_wifi esp_netif nvs_flash mqtt
                    )
