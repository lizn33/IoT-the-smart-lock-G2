# 递归查找所有的源文件（既包括C++文件也包括C文件）
FILE(GLOB_RECURSE CPP_SRC_FILES "*.cpp")
FILE(GLOB_RECURSE C_SRC_FILES "*.c")

# 确保包含所有硬件模块的源文件
set(HARDWARE_SRCS
    "oled.c"
    "rfid.c"
    "keypad.c"
    "servo.c"
    "led.c"
)

# 确保这些文件存在，否则将其从列表中移除
foreach(SRC ${HARDWARE_SRCS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
        list(APPEND VALIDATED_HARDWARE_SRCS "${SRC}")
    else()
        message(WARNING "Hardware source file ${SRC} not found, skipping")
    endif()
endforeach()

# 确保main文件被正确包含
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    set(MAIN_SRC "main.cpp")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
    set(MAIN_SRC "main.c")
else()
    message(FATAL_ERROR "No main source file found")
endif()

# 合并所有源文件
list(APPEND ALL_SRCS ${CPP_SRC_FILES} ${C_SRC_FILES} ${VALIDATED_HARDWARE_SRCS} ${MAIN_SRC})
list(REMOVE_DUPLICATES ALL_SRCS)

# 移除重复的目录路径
if(DEFINED INC_FOLDERS)
    LIST(REMOVE_DUPLICATES INC_FOLDERS)
endif()

# 注册组件，包含两个分支的所有依赖项
idf_component_register(
    SRCS ${ALL_SRCS}
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif nvs_flash mqtt esp_https_ota app_update
    PRIV_REQUIRES spi_flash
    EMBED_TXTFILES "TLS/isrgrootx1.pem"
)