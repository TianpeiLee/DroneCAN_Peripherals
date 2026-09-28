#pragma once

#include <stdint.h>

// 动态节点分配（DNA）状态与请求接口
void dna_init(void);

// 距离下一次可发送分配请求的时间戳（ms）
uint32_t dna_next_request_ms(void);

// 广播一次节点 ID 分配请求
void dna_request(uint8_t preferred_node_id);
