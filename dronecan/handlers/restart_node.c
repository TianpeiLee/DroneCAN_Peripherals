#include "handlers.h"

#include "dronecan_msgs.h"
#include "drv_tick.h"

#include "ch32v20x_conf.h"

// 收到服务后延迟一点再复位，给响应帧留出上总线的时间
#define RESTART_DELAY_MS 50u

static bool restart_pending;
static uint32_t restart_at_ms;

void dronecan_request_restart(uint32_t delay_ms)
{
    restart_at_ms = get_tick_ms() + delay_ms;
    restart_pending = true;
}

void handle_restart_node(CanardInstance *ins, CanardRxTransfer *transfer)
{
    struct uavcan_protocol_RestartNodeRequest req;

    if (uavcan_protocol_RestartNodeRequest_decode(transfer, &req))
    {
        return;
    }

    const bool ok = (req.magic_number == UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_MAGIC_NUMBER);

    struct uavcan_protocol_RestartNodeResponse res = {.ok = ok};
    uint8_t buffer[UAVCAN_PROTOCOL_RESTARTNODE_RESPONSE_MAX_SIZE];
    const uint32_t len = uavcan_protocol_RestartNodeResponse_encode(&res, buffer);

    canardReleaseRxTransferPayload(ins, transfer);

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_SIGNATURE,
                           UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_ID, &transfer->transfer_id, transfer->priority,
                           CanardResponse, buffer, len);

    if (ok)
    {
        dronecan_request_restart(RESTART_DELAY_MS);
    }
}

void dronecan_process_restart(uint32_t now_ms)
{
    if (restart_pending && (int32_t)(now_ms - restart_at_ms) >= 0)
    {
        NVIC_SystemReset();
    }
}
