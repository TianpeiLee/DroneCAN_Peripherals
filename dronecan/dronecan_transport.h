#pragma once

#include <stdint.h>

// 把 canard 发送队列中的帧交给底层 CAN 驱动
void dronecan_process_tx(void);

// 从底层 CAN 驱动取帧交给 canard
void dronecan_process_rx(uint32_t now_ms);
