#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "esp_random.h"
#include "esp_log.h"

#include "esp_wpa.h"
#include "os.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#include "esp_wifi_driver.h"

const char WPA_TAG[] = "WPA";

////////////////////////////////////////////////////////////////////////////////

bool wpa_attach(void)
{
    ESP_LOGI(WPA_TAG, "%s", __func__);
    return true;
}

bool wpa_deattach(void)
{
    ESP_LOGI(WPA_TAG, "%s", __func__);
    return true;
}

// int wpa_sta_connect(uint8_t *bssid)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// void wpa_sta_disconnected_cb(uint8_t reason_code)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
// }

// int wpa_sm_rx_eapol(u8 *src_addr, u8 *buf, u32 len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// bool wpa_sta_in_4way_handshake(void)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT

// void *hostap_init(void)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return NULL;
// }

// bool hostap_deinit(void *data)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// bool wpa_ap_join(void **sm, u8 *bssid, u8 *wpa_ie, u8 wpa_ie_len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// bool wpa_ap_remove(void *sm)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// uint8_t *wpa_ap_get_wpa_ie(uint8_t *len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return NULL;
// }

// bool wpa_ap_rx_eapol(void *hapd_data, void *sm, u8 *data, size_t data_len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// void wpa_ap_get_peer_spp_msg(void *sm, bool *spp_cap, bool *spp_req)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
// }

// #endif

// char *wpa_config_parse_string(const char *value, size_t *len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return NULL;
// }

// int wpa_parse_wpa_ie_wrapper(const u8 *wpa_ie, size_t wpa_ie_len, wifi_wpa_ie_t *data)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// #if 0

// int wpa_config_bss(u8 *bssid)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// #endif

// int wpa_michael_mic_failure(u16 is_unicast)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// #if 0

// uint8_t *wpa3_build_sae_msg(uint8_t *bssid, uint32_t type, size_t *len)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return NULL;
// }

// int wpa3_parse_sae_msg(uint8_t *buf, size_t len, uint32_t type, uint16_t status)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// int wpa_sta_rx_mgmt(u8 type, u8 *frame, size_t len, u8 *sender, u32 rssi, u8 channel, u64 current_tsf)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return 0;
// }

// #endif

void wpa_config_done(void)
{
    ESP_LOGI(WPA_TAG, "%s", __func__);
}

// #if 0

// bool wpa_sta_profile_match(u8 *bssid)
// {
//     ESP_LOGI(WPA_TAG,"%s",__func__);
//     return true;
// }

// #endif

int esp_supplicant_init(void)
{
    int ret = ESP_OK;
    struct wpa_funcs* wpa_cb;

    wpa_cb = (struct wpa_funcs*)os_zalloc(sizeof(struct wpa_funcs));
    if (!wpa_cb)
    {
        return ESP_ERR_NO_MEM;
    }

    wpa_cb->wpa_sta_init              = wpa_attach;
    wpa_cb->wpa_sta_deinit            = NULL; // wpa_deattach;
    wpa_cb->wpa_sta_rx_eapol          = NULL; // ;
    wpa_cb->wpa_sta_connect           = NULL; // wpa_sta_connect;
    wpa_cb->wpa_sta_disconnected_cb   = NULL; // wpa_sta_disconnected_cb;
    wpa_cb->wpa_sta_in_4way_handshake = NULL; // wpa_sta_in_4way_handshake;

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
    wpa_cb->wpa_ap_join             = NULL; // wpa_ap_join;
    wpa_cb->wpa_ap_remove           = NULL; // wpa_ap_remove;
    wpa_cb->wpa_ap_get_wpa_ie       = NULL; // wpa_ap_get_wpa_ie;
    wpa_cb->wpa_ap_rx_eapol         = NULL; // wpa_ap_rx_eapol;
    wpa_cb->wpa_ap_get_peer_spp_msg = NULL; // wpa_ap_get_peer_spp_msg;
    wpa_cb->wpa_ap_init             = NULL; // hostap_init;
    wpa_cb->wpa_ap_deinit           = NULL; // hostap_deinit;
#endif

    wpa_cb->wpa_config_parse_string = NULL; // wpa_config_parse_string;
    wpa_cb->wpa_parse_wpa_ie        = NULL; // wpa_parse_wpa_ie_wrapper;
    wpa_cb->wpa_config_bss          = NULL; // wpa_config_bss;
    wpa_cb->wpa_michael_mic_failure = NULL; // wpa_michael_mic_failure;
    wpa_cb->wpa_config_done         = wpa_config_done;

#ifdef CONFIG_WPA3_SAE
    esp_wifi_register_wpa3_cb(wpa_cb);
#endif
    // ret = esp_supplicant_common_init(wpa_cb);

    if (ret != 0)
    {
        return ret;
    }

    esp_wifi_register_wpa_cb_internal(wpa_cb);

#if CONFIG_WPA_WAPI_PSK
    ret = esp_wifi_internal_wapi_init();
#endif

    return ret;
}

int esp_supplicant_deinit(void)
{
    // esp_supplicant_common_deinit();
    return esp_wifi_unregister_wpa_cb_internal();
}

////////////////////////////////////////////////////////////////////////////////

int os_get_time(struct os_time* t)
{
    struct timeval tv;
    int ret = gettimeofday(&tv, NULL);
    t->sec  = (os_time_t)tv.tv_sec;
    t->usec = tv.tv_usec;
    return ret;
}

unsigned long os_random(void)
{
    return esp_random();
}

int os_get_random(unsigned char* buf, size_t len)
{
    esp_fill_random(buf, len);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// int aes_128_cbc_encrypt(const unsigned char *key, const unsigned char *iv, unsigned char *data, int data_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int aes_128_cbc_decrypt(const unsigned char *key, const unsigned char *iv, unsigned char *data, int data_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int esp_aes_wrap(const unsigned char *kek, int n, const unsigned char *plain, unsigned char *cipher)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int esp_aes_unwrap(const unsigned char *kek, int n, const unsigned char *cipher, unsigned char *plain)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int hmac_sha256_vector(const unsigned char *key, int key_len, int num_elem,
//                             const unsigned char *addr[], const int *len, unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int sha256_prf(const unsigned char *key, int key_len, const char *label,
//                             const unsigned char *data, int data_len, unsigned char *buf, int buf_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int hmac_md5(const unsigned char *key, unsigned int key_len, const unsigned char *data,
//                               unsigned int data_len, unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int hmac_md5_vector(const unsigned char *key, unsigned int key_len, unsigned int num_elem,
//                               const unsigned char *addr[], const unsigned int *len, unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int hmac_sha1(const unsigned char *key, unsigned int key_len, const unsigned char *data,
//                               unsigned int data_len, unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int hmac_sha1_vector(const unsigned char *key, unsigned int key_len, unsigned int num_elem,
//                               const unsigned char *addr[], const unsigned int *len, unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int sha1_prf(const unsigned char *key, unsigned int key_len, const char *label,
//                               const unsigned char *data, unsigned int data_len, unsigned char *buf, unsigned int
//                               buf_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int sha1_vector(unsigned int num_elem, const unsigned char *addr[], const unsigned int *len,
//                               unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int pbkdf2_sha1(const char *passphrase, const char *ssid, unsigned int ssid_len,
//                               int iterations, unsigned char *buf, unsigned int buflen)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int rc4_skip(const unsigned char *key, unsigned int keylen, unsigned int skip,
//                               unsigned char *data, unsigned int data_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// int md5_vector(unsigned int num_elem, const unsigned char *addr[], const unsigned int *len,
//                               unsigned char *mac)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// void esp_aes_encrypt(void *ctx, const unsigned char *plain, unsigned char *crypt)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
// }

// void * aes_encrypt_init(const unsigned char *key,  unsigned int len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return NULL;
// }

// void aes_encrypt_deinit(void *ctx)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
// }

// void esp_aes_decrypt(void *ctx, const unsigned char *crypt, unsigned char *plain)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
// }

// void * aes_decrypt_init(const unsigned char *key, unsigned int len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return NULL;
// }

// void aes_decrypt_deinit(void *ctx)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
// }

// int omac1_aes_128(const uint8_t *key, const uint8_t *data, size_t data_len,
//                                    uint8_t *mic)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

// uint8_t * ccmp_decrypt(const uint8_t *tk, const uint8_t *ieee80211_hdr,
//                                         const uint8_t *data, size_t data_len,
//                                         size_t *decrypted_len, bool espnow_pkt)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return NULL;
// }

// uint8_t * ccmp_encrypt(const uint8_t *tk, uint8_t *frame, size_t len, size_t hdrlen,
//                                         uint8_t *pn, int keyid, size_t *encrypted_len)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return NULL;
// }

// int esp_aes_gmac(const uint8_t *key, size_t keylen, const uint8_t *iv, size_t iv_len,
//                               const uint8_t *aad, size_t aad_len, uint8_t *mic)
// {
//     ESP_LOGI(WPA_TAG, "%s", __func__);
//     return 0;
// }

/*
 * This structure is used to set the cyrpto callback function for station to connect when in security mode.
 * These functions either call MbedTLS API's if USE_MBEDTLS_CRYPTO flag is set through Kconfig, or native
 * API's otherwise. We recommend setting the flag since MbedTLS API's utilize hardware acceleration while
 * native API's are use software implementations.
 */
const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs = {
    .size               = sizeof(wpa_crypto_funcs_t),
    .version            = ESP_WIFI_CRYPTO_VERSION,
    .aes_wrap           = NULL, // (esp_aes_wrap_t)esp_aes_wrap,
    .aes_unwrap         = NULL, // (esp_aes_unwrap_t)esp_aes_unwrap,
    .hmac_sha256_vector = NULL, // (esp_hmac_sha256_vector_t)hmac_sha256_vector,
    .sha256_prf         = NULL, // (esp_sha256_prf_t)sha256_prf,
    .hmac_md5           = NULL, // (esp_hmac_md5_t)hmac_md5,
    .hamc_md5_vector    = NULL, // (esp_hmac_md5_vector_t)hmac_md5_vector,
    .hmac_sha1          = NULL, // (esp_hmac_sha1_t)hmac_sha1,
    .hmac_sha1_vector   = NULL, // (esp_hmac_sha1_vector_t)hmac_sha1_vector,
    .sha1_prf           = NULL, // (esp_sha1_prf_t)sha1_prf,
    .sha1_vector        = NULL, // (esp_sha1_vector_t)sha1_vector,
    .pbkdf2_sha1        = NULL, // (esp_pbkdf2_sha1_t)pbkdf2_sha1,
    .rc4_skip           = NULL, // (esp_rc4_skip_t)rc4_skip,
    .md5_vector         = NULL, // (esp_md5_vector_t)md5_vector,
    .aes_encrypt        = NULL, // (esp_aes_encrypt_t)esp_aes_encrypt,
    .aes_encrypt_init   = NULL, // (esp_aes_encrypt_init_t)aes_encrypt_init,
    .aes_encrypt_deinit = NULL, // (esp_aes_encrypt_deinit_t)aes_encrypt_deinit,
    .aes_decrypt        = NULL, // (esp_aes_decrypt_t)esp_aes_decrypt,
    .aes_decrypt_init   = NULL, // (esp_aes_decrypt_init_t)aes_decrypt_init,
    .aes_decrypt_deinit = NULL, // (esp_aes_decrypt_deinit_t)aes_decrypt_deinit,
    .aes_128_encrypt    = NULL, // (esp_aes_128_encrypt_t)aes_128_cbc_encrypt,
    .aes_128_decrypt    = NULL, // (esp_aes_128_decrypt_t)aes_128_cbc_decrypt,
    .omac1_aes_128      = NULL, // (esp_omac1_aes_128_t)omac1_aes_128,
    .ccmp_decrypt       = NULL, // (esp_ccmp_decrypt_t)ccmp_decrypt,
    .ccmp_encrypt       = NULL, // (esp_ccmp_encrypt_t)ccmp_encrypt,
    .aes_gmac           = NULL, // (esp_aes_gmac_t)esp_aes_gmac,
};

const mesh_crypto_funcs_t g_wifi_default_mesh_crypto_funcs = {
    .aes_128_encrypt = NULL, // (esp_aes_128_encrypt_t)aes_128_cbc_encrypt,
    .aes_128_decrypt = NULL, // (esp_aes_128_decrypt_t)aes_128_cbc_decrypt,
};
