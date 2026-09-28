#include "dronecan_transport.h"

#include "dronecan_node.h"
#include "drv_can.h"

#include <string.h>

void dronecan_process_tx(void)
{
    const CanardCANFrame *txf;
    while ((txf = canardPeekTxQueue(&canard)) != NULL)
    {
        if (!drv_can_transmit(txf->id, txf->data, txf->data_len))
        {
            break;
        }
        canardPopTxQueue(&canard);
    }
}

void dronecan_process_rx(uint32_t now_ms)
{
    uint32_t id;
    uint8_t data[8];
    uint8_t len;

    while (drv_can_receive(&id, data, &len))
    {
        CanardCANFrame frame = {
            .id = id | CANARD_CAN_FRAME_EFF,
            .data_len = len,
        };
        memcpy(frame.data, data, len);
        canardHandleRxFrame(&canard, &frame, (uint64_t)now_ms * 1000);
    }
}
