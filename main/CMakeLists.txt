# Recursively find all source files (both C++ and C files)
FILE(GLOB_RECURSE CPP_SRC_FILES "*.cpp")
FILE(GLOB_RECURSE C_SRC_FILES "*.c")

# Add MQTT wrapper source explicitly (create this file if it doesn't exist yet)
set(CPP_WRAPPER_SRCS
    "mqtt_wrappers.cpp"
)

# Check if wrapper file exists, if not create placeholder to avoid build errors
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mqtt_wrappers.cpp")
    message(STATUS "Creating placeholder mqtt_wrappers.cpp file")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/mqtt_wrappers.cpp" "
// Placeholder MQTT wrapper functions - replace with actual implementation
#include \"mqtt_wrappers.h\"
#include <string>

#ifdef __cplusplus
extern \"C\" {
#endif

void mqtt_init(const char* broker_uri, const char* client_id, 
               const char* username, const char* password) {
    // Implementation will go here
}

void wifi_init(const char* ssid, const char* password) {
    // Implementation will go here
}

void mqtt_send_lock_status(const char* device_id, bool is_locked) {
    // Implementation will go here
}

void mqtt_send_alert(const char* device_id, const char* alert_type) {
    // Implementation will go here
}

void init_device_id(char* out_device_id, int max_len) {
    // Implementation will go here
}

#ifdef __cplusplus
}
#endif
")
endif()

# Check if wrapper header exists, if not create it
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mqtt_wrappers.h")
    message(STATUS "Creating placeholder mqtt_wrappers.h file")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/mqtt_wrappers.h" "
#ifndef MQTT_WRAPPERS_H
#define MQTT_WRAPPERS_H

#include <stdbool.h>

#ifdef __cplusplus
extern \"C\" {
#endif

// Initialize MQTT system
void mqtt_init(const char* broker_uri, const char* client_id, 
               const char* username, const char* password);

// Initialize WiFi
void wifi_init(const char* ssid, const char* password);

// Send door lock status
void mqtt_send_lock_status(const char* device_id, bool is_locked);

// Send alert message
void mqtt_send_alert(const char* device_id, const char* alert_type);

// Get device ID (can be derived from MAC address)
void init_device_id(char* out_device_id, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_WRAPPERS_H */
")
endif()

# Merge all source files
list(APPEND ALL_SRCS ${CPP_SRC_FILES} ${C_SRC_FILES} ${CPP_WRAPPER_SRCS})
list(REMOVE_DUPLICATES ALL_SRCS)

# Print source files for debugging
message(STATUS "Source files found: ${ALL_SRCS}")

# Register component with all dependencies
idf_component_register(
    SRCS ${ALL_SRCS}
    INCLUDE_DIRS "."
    REQUIRES esp_wifi 
            esp_netif 
            nvs_flash 
            mqtt 
            esp_https_ota 
            app_update 
            esp_driver_gpio      # Required for GPIO operations
            driver               # General driver support
            esp_timer            # Required for timer operations
            esp_driver_spi       # Required for SPI (RFID)
            esp_driver_i2c       # Required for I2C (OLED)
            esp_driver_ledc      # Required for Servo PWM
            esp_driver_uart      # Required for serial communications
            esp_adc             # Required for ADC (keypad)
            esp_hw_support      # Basic hardware support
            esp_system          # System functions
            esp_event           # Event handling
    PRIV_REQUIRES spi_flash
    EMBED_TXTFILES "TLS/isrgrootx1.pem"
)

# Compiler flags specific to C++ files
if(CMAKE_CXX_COMPILER_ID)
    # Add C++ specific flags
    set_source_files_properties(
        ${CPP_SRC_FILES} ${CPP_WRAPPER_SRCS}
        PROPERTIES COMPILE_FLAGS "-std=gnu++17 -fno-rtti"
    )
endif()