#include "device_identity.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <cstdint>
#include <cstdio>

namespace aqm
{
esp_err_t load_device_identity(char (&serial)[37])
{
    serial[0] = '\0';
    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) return error; // Never erase storage as automatic recovery.
    nvs_handle_t handle;
    error = nvs_open("aqm_identity", NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;
    uint8_t uuid[16] = {};
    size_t size = sizeof(uuid);
    error = nvs_get_blob(handle, "uuid", uuid, &size);
    if (error == ESP_ERR_NVS_NOT_FOUND)
    {
        bootloader_random_enable();
        esp_fill_random(uuid, sizeof(uuid));
        bootloader_random_disable();
        uuid[6] = (uuid[6] & 0x0f) | 0x40;
        uuid[8] = (uuid[8] & 0x3f) | 0x80;
        error = nvs_set_blob(handle, "uuid", uuid, sizeof(uuid));
        if (error == ESP_OK) error = nvs_commit(handle);
    }
    else if (error == ESP_OK &&
             (size != sizeof(uuid) || (uuid[6] & 0xf0) != 0x40 || (uuid[8] & 0xc0) != 0x80))
    {
        error = ESP_ERR_INVALID_RESPONSE;
    }
    nvs_close(handle);
    if (error != ESP_OK) return error;
    std::snprintf(serial, sizeof(serial),
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
                  uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return ESP_OK;
}
}
