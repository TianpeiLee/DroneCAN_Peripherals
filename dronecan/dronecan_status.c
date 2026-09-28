#include "dronecan_status.h"

struct uavcan_protocol_NodeStatus dronecan_node_status;

static bool software_update_mode;

void dronecan_set_software_update_mode(void)
{
    software_update_mode = true;
}

void dronecan_update_node_status(uint32_t now_ms)
{
    dronecan_node_status.uptime_sec = now_ms / 1000;
    dronecan_node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    dronecan_node_status.mode = software_update_mode ? UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE
                                                     : UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    dronecan_node_status.sub_mode = 0;
    dronecan_node_status.vendor_specific_status_code = 0;
}
