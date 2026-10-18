#include "bme280_sensor.h"
#include "shared_i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

extern "C" void app_main(void)
{
    constexpr const char *tag = "aqm";
    i2c_master_bus_handle_t bus = nullptr;
    esp_err_t error = aqm::create_i2c_bus(CONFIG_AQM_I2C_SDA_GPIO, CONFIG_AQM_I2C_SCL_GPIO, &bus);
    if (error != ESP_OK)
    {
        ESP_LOGE(tag, "I2C startup failed (%s). Set distinct output-capable SDA/SCL GPIOs in menuconfig > Air Quality Monitor and check wiring.", esp_err_to_name(error));
        return;
    }
#ifdef CONFIG_AQM_BME280_ADDRESS_77
    constexpr uint8_t address = 0x77;
#else
    constexpr uint8_t address = 0x76;
#endif
    aqm::Bme280 sensor(bus, address);
    int64_t scheduled = esp_timer_get_time();
    while (true)
    {
        aqm::Measurement sample;
        error = sensor.cycle(sample);
        if (error != ESP_OK) ESP_LOGW(tag, "BME280 cycle failed: %s; retry next cycle", esp_err_to_name(error));
        char line[192];
        aqm::format_measurement(sample, line, sizeof(line));
        ESP_LOGI(tag, "%s", line);
        scheduled = aqm::next_deadline(scheduled, esp_timer_get_time());
        int64_t remaining;
        while ((remaining = scheduled - esp_timer_get_time()) > 0)
        {
            const int64_t tick_us = portTICK_PERIOD_MS * 1000;
            vTaskDelay(static_cast<TickType_t>((remaining + tick_us - 1) / tick_us));
        }
    }
}
