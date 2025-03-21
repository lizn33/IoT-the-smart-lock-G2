# 递归查找所有的 .cpp 源文件
FILE(GLOB_RECURSE SRC_FILES "*.cpp")

# 移除重复的目录路径
LIST(REMOVE_DUPLICATES INC_FOLDERS)
idf_component_register(SRCS  ${SRC_FILES}
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_netif nvs_flash mqtt
                    )
