/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    bsp_log.h
 * @brief   This file contains all the function prototypes for
 *          the bsp_log.c file
 ******************************************************************************
 */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BSP_LOG_H__
#define __BSP_LOG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

/* Defines ------------------------------------------------------------------*/
#define LOG_IN_RTOS (1U)


    /* Typedef ------------------------------------------------------------------*/

    enum
    {
        LOG_STATUS_OK = 0,
        LOG_STATUS_FAIL,
        LOG_STATUS_BUSY,
        LOG_STATUS_ERROR,
        LOG_STATUS_TIMEOUT
    };

    /* Function Prototypes ------------------------------------------------------------------*/

#if LOG_IN_RTOS
    extern uint8_t log_init(void *memory_ptr);
    extern void log_recycle(void);
#else
extern uint8_t log_init(void);
#endif

    extern int log_printf(const char *format, ...);
    extern uint8_t log_port_lock(void);
    extern uint8_t log_port_try_lock(void);
    extern uint8_t log_port_unlock(void);
    extern uint8_t log_port_unlock_fromISR(void);
    extern uint8_t log_port_wait_unlock(uint32_t waitTick);
		extern HAL_StatusTypeDef log_receive_with_idle(uint16_t wait_tick);
		extern HAL_StatusTypeDef log_save_receive(uint8_t* src, uint16_t length, uint16_t wait_tick);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_LOG_H__ */
