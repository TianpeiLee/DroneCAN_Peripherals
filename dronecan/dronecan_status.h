#pragma once

#include <stdint.h>

#include "dronecan_msgs.h"

extern struct uavcan_protocol_NodeStatus dronecan_node_status;

void dronecan_update_node_status(uint32_t now_ms);

// 置位后 NodeStatus.mode 上报 MODE_SOFTWARE_UPDATE（固件升级期间）
void dronecan_set_software_update_mode(void);
