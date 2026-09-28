#include "dronecan_publishers.h"

#include "dronecan_node.h"
#include "dronecan_status.h"

#include "dronecan_msgs.h"

void dronecan_broadcast_node_status(void)
{
    uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
    uint32_t len = uavcan_protocol_NodeStatus_encode(&dronecan_node_status, buffer);

    static uint8_t transfer_id = 0;
    canardBroadcast(&canard, UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE, UAVCAN_PROTOCOL_NODESTATUS_ID, &transfer_id,
                    CANARD_TRANSFER_PRIORITY_LOW, buffer, len);
}

void dronecan_publish_1hz(uint32_t now_ms)
{
    static uint32_t last_1hz_ms = 0;
    if (now_ms - last_1hz_ms < 1000)
    {
        return;
    }
    last_1hz_ms = now_ms;

    // 填充并编码 NodeStatus
    dronecan_update_node_status(now_ms);
    dronecan_broadcast_node_status();
}
