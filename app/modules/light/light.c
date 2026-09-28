#include "light.h"

#include "app_config.h"

#include "dronecan_msgs.h"
#include <stdio.h>

// 每个 WS2812 一项；index 默认等于数组位置，可按实际接线顺序重排
static ws2812_led_t leds[APP_WS2812_LED_COUNT];

static uint8_t *ws2812_buffer;

bool light_init(void)
{
    ws2812_buffer = drv_ws2812_init(APP_WS2812_LED_COUNT);
    if (ws2812_buffer == NULL)
    {
        return false;
    }

    for (uint32_t i = 0; i < APP_WS2812_LED_COUNT; i++)
    {
        leds[i].index = (uint8_t)i;
        leds[i].color = (color_fmt){0, 0, 0};
    }
    leds[0].color = (color_fmt){255, 0, 0};
    
    return true;
}

void light_handle_lights_command(CanardInstance *ins, CanardRxTransfer *transfer)
{
    (void)ins;

    struct uavcan_equipment_indication_LightsCommand msg;
    uavcan_equipment_indication_LightsCommand_decode(transfer, &msg);

    for (size_t i = 0; i < msg.commands.len; i++)
    {
        const uint8_t light_id = msg.commands.data[i].light_id;

        for (uint32_t j = 0; j < APP_WS2812_LED_COUNT; j++)
        {
            if (leds[j].index != light_id)
            {
                continue;
            }

            leds[j].color.r = msg.commands.data[i].color.red;
            leds[j].color.g = msg.commands.data[i].color.green;
            leds[j].color.b = msg.commands.data[i].color.blue;

            drv_ws2812_draw(leds[j].color, ws2812_buffer, j, 1);
            printf("%d %#x %#x %#x\n", light_id, msg.commands.data[i].color.red, msg.commands.data[i].color.green,
                   msg.commands.data[i].color.blue);
        }
    }
}
