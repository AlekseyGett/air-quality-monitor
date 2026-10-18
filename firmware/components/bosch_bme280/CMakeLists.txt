idf_component_register(SRCS "bme280.c" INCLUDE_DIRS "include")
target_compile_definitions(${COMPONENT_LIB} PUBLIC BME280_DOUBLE_ENABLE)
