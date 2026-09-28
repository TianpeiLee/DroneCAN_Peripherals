#pragma once

#include <stdint.h>

// 周期广播任务
void dronecan_publish_1hz(uint32_t now_ms);

// 立即广播一次 NodeStatus（需先调用 dronecan_update_node_status 填充）
void dronecan_broadcast_node_status(void);
