idf_component_register(SRCS "bme280_sensor.cpp" "measurement.cpp" INCLUDE_DIRS "include" REQUIRES bosch_bme280 esp_driver_i2c esp_timer freertos)
