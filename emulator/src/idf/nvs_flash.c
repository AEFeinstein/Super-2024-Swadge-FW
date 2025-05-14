#include "nvs.h"
#include "nvs_flash.h"
#include "emu_main.h"

////////////////////////////////////////////////////////////////
// nvs_flash.h
////////////////////////////////////////////////////////////////

esp_err_t nvs_flash_init(void)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_init_partition(const char* partition_label)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_deinit(void)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_deinit_partition(const char* partition_label)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_erase_partition(const char* part_name)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_secure_init(nvs_sec_cfg_t* cfg)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_secure_init_partition(const char* partition_label, nvs_sec_cfg_t* cfg)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_register_security_scheme(nvs_sec_scheme_t* scheme_cfg)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

nvs_sec_scheme_t* nvs_flash_get_default_security_scheme(void)
{
    WARN_UNIMPLEMENTED();
    return NULL;
}

esp_err_t nvs_flash_generate_keys_v2(nvs_sec_scheme_t* scheme_cfg, nvs_sec_cfg_t* cfg)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_flash_read_security_cfg_v2(nvs_sec_scheme_t* scheme_cfg, nvs_sec_cfg_t* cfg)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

////////////////////////////////////////////////////////////////
// nvs.h
////////////////////////////////////////////////////////////////

esp_err_t nvs_open(const char* namespace_name, nvs_open_mode_t open_mode, nvs_handle_t* out_handle)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_open_from_partition(const char* part_name, const char* namespace_name, nvs_open_mode_t open_mode,
                                  nvs_handle_t* out_handle)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_i8(nvs_handle_t handle, const char* key, int8_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_i64(nvs_handle_t handle, const char* key, int64_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_u64(nvs_handle_t handle, const char* key, uint64_t value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char* key, const char* value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_i8(nvs_handle_t handle, const char* key, int8_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_i16(nvs_handle_t handle, const char* key, int16_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_i64(nvs_handle_t handle, const char* key, int64_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_u64(nvs_handle_t handle, const char* key, uint64_t* out_value)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char* key, char* out_value, size_t* length)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_find_key(nvs_handle_t handle, const char* key, nvs_type_t* out_type)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    WARN_UNIMPLEMENTED();
}

esp_err_t nvs_get_stats(const char* part_name, nvs_stats_t* nvs_stats)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_get_used_entry_count(nvs_handle_t handle, size_t* used_entries)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_entry_find(const char* part_name, const char* namespace_name, nvs_type_t type,
                         nvs_iterator_t* output_iterator)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_entry_find_in_handle(nvs_handle_t handle, nvs_type_t type, nvs_iterator_t* output_iterator)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_entry_next(nvs_iterator_t* iterator)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

esp_err_t nvs_entry_info(const nvs_iterator_t iterator, nvs_entry_info_t* out_info)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

void nvs_release_iterator(nvs_iterator_t iterator)
{
    WARN_UNIMPLEMENTED();
}
