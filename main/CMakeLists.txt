# 递归查找所有的 .cpp 源文件
FILE(GLOB_RECURSE SRC_FILES "*.cpp")

# # 递归查找所有的 .h 头文件所在目录
# FILE(GLOB_RECURSE HEADER_FILES "*.h")

# # 提取头文件所在的目录
# SET(INC_FOLDERS "")
# FOREACH(file ${HEADER_FILES})
#     GET_FILENAME_COMPONENT(dir ${file} DIRECTORY)
#     LIST(APPEND INC_FOLDERS ${dir})
# ENDFOREACH()

# 移除重复的目录路径
LIST(REMOVE_DUPLICATES INC_FOLDERS)
idf_component_register(SRCS  ${SRC_FILES}
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_netif nvs_flash mqtt
                    )
