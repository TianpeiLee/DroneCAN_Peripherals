#include "app_params.h"

#include "dronecan_params.h"

#include "esc.h"

#include "ch32v20x_conf.h"

volatile bool led_red = 0;

static const dronecan_param_t app_parameters[] = {
    {.name = "LED_RED",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE,
     .value = (void *)&led_red,
     .min_value = 0,
     .max_value = 1},
    {.name = "ESC_INDEX",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE,
     .value = (void *)&esc_index,
     .min_value = 0,
     .max_value = 31},
    {.name = "USE_DSHOT",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE,
     .value = (void *)&use_dshot,
     .min_value = 0,
     .max_value = 1},
    {.name = "POLE_PAIRS",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE,
     .value = (void *)&esc_pole_pairs,
     .min_value = 1,
     .max_value = 63},
    {.name = "EDT_ENABLE",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE,
     .value = (void *)&esc_edt_enable,
     .min_value = 0,
     .max_value = 1},
    {.name = "DSHOT_RATE",
     .type = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE,
     .value = (void *)&esc_dshot_rate_hz,
     .min_value = 50,
     .max_value = 8000},
};

static void app_param_changed(uint32_t index)
{
    switch (index)
    {
    case 0: // LED_RED
        GPIO_WriteBit(GPIOA, GPIO_Pin_7, (!led_red) ? Bit_SET : Bit_RESET);
        break;
    default:
        break;
    }
}

void app_params_init(void)
{
    dronecan_params_register(app_parameters, sizeof(app_parameters) / sizeof(app_parameters[0]), app_param_changed);
}
