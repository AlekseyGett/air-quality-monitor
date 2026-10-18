idf_component_register(SRCS "air-quality-monitor.cpp" INCLUDE_DIRS "."
                       REQUIRES shared_i2c bme280_sensor esp_timer)
