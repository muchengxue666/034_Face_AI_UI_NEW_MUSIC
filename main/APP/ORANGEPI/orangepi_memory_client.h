/**
 ******************************************************************************
 * @file        orangepi_memory_client.h
 * @brief       香橙派家庭记忆中枢客户端
 ******************************************************************************
 */

#ifndef __ORANGEPI_MEMORY_CLIENT_H
#define __ORANGEPI_MEMORY_CLIENT_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ORANGEPI_SYSTEM_PROMPT_MAX   1200
#define ORANGEPI_TRACE_ID_MAX        64

typedef struct {
    bool success;                                   /* 请求是否成功 */
    bool has_memory;                                /* 是否命中家庭记忆 */
    bool degraded;                                  /* 服务是否处于降级模式 */
    int elapsed_ms;                                 /* 服务端耗时 */
    char trace_id[ORANGEPI_TRACE_ID_MAX];           /* 追踪 ID */
    char system_prompt[ORANGEPI_SYSTEM_PROMPT_MAX + 1];
} orangepi_compose_result_t;

void orangepi_memory_client_init(void);
bool orangepi_memory_client_probe(void);
bool orangepi_memory_client_is_available(void);
bool orangepi_memory_client_compose_prompt(const char *query, orangepi_compose_result_t *out_result);
esp_err_t orangepi_memory_client_upsert_note(const char *sender, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* __ORANGEPI_MEMORY_CLIENT_H */
