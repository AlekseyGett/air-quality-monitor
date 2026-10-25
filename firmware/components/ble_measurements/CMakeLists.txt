idf_component_register(SRCS "measurement_snapshot.cpp" "generated/measurements.pb.c" "ble_measurements.cpp"
                       INCLUDE_DIRS "include" "generated"
                       REQUIRES bme280_sensor nanopb device_identity bt esp_timer)
