#include "debug.h"
#include "dronecan_msgs.h"
#include <stdio.h>

#include "app_config.h"
#include "app_params.h"

#include "dronecan_dispatch.h"
#include "dronecan_node.h"
#include "dronecan_publishers.h"
#include "dronecan_transport.h"
#include "handlers/dna.h"

#include "drv_can.h"
#include "drv_tick.h"

#include "esc.h"
#include "light.h"

#include "ch32v20x_conf.h"

static const dn_dispatch_entry_t app_dispatch[] = {
    {UAVCAN_EQUIPMENT_INDICATION_LIGHTSCOMMAND_ID, CanardTransferTypeBroadcast,
     UAVCAN_EQUIPMENT_INDICATION_LIGHTSCOMMAND_SIGNATURE, light_handle_lights_command},
    {UAVCAN_EQUIPMENT_ESC_RAWCOMMAND_ID, CanardTransferTypeBroadcast, UAVCAN_EQUIPMENT_ESC_RAWCOMMAND_SIGNATURE,
     esc_handle_raw_command},
};

static void extra_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_Out_PP, .GPIO_Pin = GPIO_Pin_7, .GPIO_Speed = GPIO_Speed_50MHz};

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_WriteBit(GPIOA, GPIO_Pin_7, !led_red);
}

int main(void)
{
    SystemCoreClockUpdate();
    USART_Printf_Init(1000000);
    printf("Hello, from %s!\n\tComplier: %s\n\tCompiler time: %s\n\tSysclk: %ld\n", APP_NAME, __VERSION__, __DATE__,
           SystemCoreClock);

    if (!light_init())
    {
        printf("WS2812 buffer allocation failed\n");
        while (1)
            ;
    }

    drv_tick_init();
    extra_init();
    app_params_init();
    esc_init();
    if (!drv_can_init())
    {
        while (1)
            ;
    }

    if (!dronecan_node_init(CANARD_BROADCAST_NODE_ID))
    {
        while (1)
            ;
    }

    dn_dispatch_register(app_dispatch, sizeof(app_dispatch) / sizeof(app_dispatch[0]));

    uint32_t now_ms = 0;

    while (1)
    {
        if (canardGetLocalNodeID(&canard) == CANARD_BROADCAST_NODE_ID)
        {
            if ((uint32_t)get_tick_ms() > dna_next_request_ms())
            {
                dna_request(APP_NODE_ID_PREFERRED);
            }
        }

        now_ms = get_tick_ms();
        dronecan_process_rx(now_ms);
        dronecan_process_tx();
        esc_update();

        dronecan_process_restart(now_ms);

        dronecan_publish_1hz(now_ms);
    }
}
