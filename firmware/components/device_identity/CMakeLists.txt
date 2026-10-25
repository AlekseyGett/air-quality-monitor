idf_component_register(SRCS "device_identity.cpp" INCLUDE_DIRS "include"
                       REQUIRES nvs_flash bootloader_support esp_hw_support)
