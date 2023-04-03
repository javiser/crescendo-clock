
#ifndef _INCLUDE_CLOCK_COMMON_HPP_
#define _INCLUDE_CLOCK_COMMON_HPP_

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#define ROT_ENC_A_GPIO      GPIO_NUM_4
#define ROT_ENC_B_GPIO      GPIO_NUM_5
#define ROT_ENC_BUTTON_GPIO GPIO_NUM_3
#define ROT_ENC_BUTTON_INVERTED      1

#define DISPLAY_MOSI_GPIO   GPIO_NUM_8
#define DISPLAY_SCLK_GPIO   GPIO_NUM_9
#define DISPLAY_DC_GPIO     GPIO_NUM_10
#define DISPLAY_RST_GPIO    GPIO_NUM_7
#define DISPLAY_BL_GPIO     GPIO_NUM_6

#define MP3_PLAYER_UART_PORT_NUM     1
#define MP3_PLAYER_TX       GPIO_NUM_21
#define MP3_PLAYER_RX       GPIO_NUM_20

#define LIGHT_ADC_CHANNEL   ADC_CHANNEL_2  // = GPIO2
#define LIGHT_ADC_ATTEN     ADC_ATTEN_DB_0

typedef struct {
    uint8_t hour;
    uint8_t minute;
} clock_time_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_credentials_t;

#endif // _INCLUDE_CLOCK_COMMON_HPP_