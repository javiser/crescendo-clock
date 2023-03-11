#ifndef _INCLUDE_LGFX_ILI9341_HPP
#define _INCLUDE_LGFX_ILI9341_HPP

#include <lgfx/v1_init.hpp>
#include "clock_common.hpp"

#define LGFX_USE_V1

class LGFX_ILI9341 : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX_ILI9341(void)
    {
        {
            auto cfg = _bus_instance.config();

            //cfg.spi_host = VSPI_HOST;             // Commented out because of ESP32C3
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;              // SPI clock on transmission (up to 80MHz, rounded to 80MHz divided by an integer)
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;                    // Transaction lock
            cfg.dma_channel = 0;                    // Set the DMA channel (1 or 2. 0=disable)
            cfg.pin_sclk = DISPLAY_SCLK_GPIO;
            cfg.pin_mosi = DISPLAY_MOSI_GPIO;
            cfg.pin_miso = -1;                      // (-1 = disable)
            cfg.pin_dc = DISPLAY_DC_GPIO;           // (-1 = disable)
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = -1;                     // (-1 = disable)
            cfg.pin_rst = DISPLAY_RST_GPIO;      // (-1 = disable)
            cfg.pin_busy = -1;                   // (-1 = disable)

            cfg.memory_width = 240;     // Maximum width supported by driver IC     
            cfg.memory_height = 320;    // Maximum height supported by driver IC
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 3;    // Offset of values in the direction of rotation 0 ~ 7 (4 ~ 7 are upside down) 
            cfg.dummy_read_pixel = 8;   // Number of bits of dummy read before reading pixels
            cfg.dummy_read_bits = 1;    // Number of bits of dummy read before reading data other than pixels
            cfg.readable = true;        // Set to true if data can be read
            cfg.invert = false;          // Set to true if the light and darkness of the panel is reversed
            cfg.rgb_order = false;      // Set to true if the red and blue of the panel are swapped
            cfg.dlen_16bit = false;     // Set to true for panels that send data length in 16-bit units
            cfg.bus_shared = false;     // If the bus is shared with the SD card, set to true (bus control is performed with drawJpgFile etc.)

            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();

            cfg.pin_bl = DISPLAY_BL_GPIO;
            cfg.invert = false;         // True if you want to invert the brightness of the backlight
            cfg.freq = 44100;           // Backlight PWM frequency
            cfg.pwm_channel = 0;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};

#endif // _INCLUDE_LGFX_ILI9341_HPP