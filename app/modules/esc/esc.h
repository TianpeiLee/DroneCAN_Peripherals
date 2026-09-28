#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "canard.h"

#define EDT_CMD_ENABLE 13
#define EDT_BURST_FRAMES 6
#define EDT_RETRY_MS 500u

extern uint64_t esc_index;
extern uint64_t esc_pole_pairs;
extern uint64_t esc_dshot_rate_hz;
extern bool use_dshot;
extern bool esc_edt_enable;

void esc_init(void);
void esc_handle_raw_command(CanardInstance *ins, CanardRxTransfer *transfer);

// 主循环周期调用：轮询回传采集并输出（当前为原始打印）
void esc_update(void);