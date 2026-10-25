#include "device_identity.h"
#include "nvs.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
namespace
{
std::array<unsigned char, 16> stored;
size_t stored_size;
bool exists, entropy;
int failure, generated, writes, commits, closes;
void reset()
{
    stored.fill(0); stored_size = 16; exists = entropy = false;
    failure = generated = writes = commits = closes = 0;
}
}
esp_err_t nvs_flash_init() { return failure == 1 ? ESP_FAIL : ESP_OK; }
esp_err_t nvs_open(const char *name, int, nvs_handle_t *h)
{
    assert(!std::strcmp(name, "aqm_identity")); *h = 1;
    return failure == 2 ? ESP_FAIL : ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t, const char *key, void *out, size_t *size)
{
    assert(!std::strcmp(key, "uuid"));
    if (failure == 3) return ESP_FAIL;
    if (!exists) return ESP_ERR_NVS_NOT_FOUND;
    if (stored_size > *size) return ESP_ERR_INVALID_SIZE;
    *size = stored_size; std::memcpy(out, stored.data(), *size); return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *data, size_t size)
{
    ++writes; assert(size == 16);
    if (failure == 4) return ESP_FAIL;
    std::memcpy(stored.data(), data, size); return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t)
{
    ++commits; if (failure == 5) return ESP_FAIL;
    exists = true; return ESP_OK;
}
void nvs_close(nvs_handle_t) { ++closes; }
void bootloader_random_enable() { assert(!entropy); entropy = true; }
void bootloader_random_disable() { assert(entropy); entropy = false; }
void esp_fill_random(void *out, size_t size)
{
    assert(entropy); ++generated;
    for (size_t i = 0; i < size; ++i) static_cast<unsigned char *>(out)[i] = i;
}
int main()
{
    reset(); char serial[37];
    assert(aqm::load_device_identity(serial) == ESP_OK);
    assert(!std::strcmp(serial, "00010203-0405-4607-8809-0a0b0c0d0e0f"));
    assert(generated == 1 && writes == 1 && commits == 1 && closes == 1 && !entropy);
    char again[37]; assert(aqm::load_device_identity(again) == ESP_OK);
    assert(!std::strcmp(serial, again) && generated == 1 && writes == 1);
    for (int f = 1; f <= 5; ++f)
    {
        reset(); failure = f; std::memset(serial, 'x', sizeof(serial));
        assert(aqm::load_device_identity(serial) != ESP_OK && serial[0] == 0 && !entropy);
        assert(generated == (f >= 4 ? 1 : 0));
    }
    for (int corrupt = 0; corrupt < 4; ++corrupt)
    {
        reset(); exists = true; stored[6] = 0x40; stored[8] = 0x80;
        if (corrupt == 0) stored_size = 15;
        if (corrupt == 1) stored_size = 17;
        if (corrupt == 2) stored[6] = 0x10;
        if (corrupt == 3) stored[8] = 0xc0;
        assert(aqm::load_device_identity(serial) != ESP_OK && serial[0] == 0);
        assert(generated == 0 && writes == 0 && commits == 0 && closes == 1);
    }
    std::puts("Identity tests passed");
}
