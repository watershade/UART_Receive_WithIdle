/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    bsp_common.h
 * @brief   This file contains all the function prototypes for
 *          the bsp_common.c file
 ******************************************************************************
 */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BSP_COMMON_H__
#define __BSP_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes ------------------------------------------------------------------*/
    #include "usart.h"
		#include "tx_api.h"
    /* Defines ------------------------------------------------------------------*/
    #define LOG_UART_PORT huart3
	
		#define EVT_FLAG_LOG_RXD_PAUSED  		(0x0001U)

	
    /* Typedef ------------------------------------------------------------------*/
    enum
    {
        BSP_STATUS_OK = 0,
        BSP_STATUS_FAIL,
        BSP_STATUS_BUSY,
        BSP_STATUS_ERROR,
        BSP_STATUS_TIMEOUT
    };
		

		/* Variables ------------------------------------------------------------------*/
		extern TX_EVENT_FLAGS_GROUP bsp_evt_group;

    /* Function Prototypes ------------------------------------------------------------------*/
		extern HAL_StatusTypeDef bsp_common_init(void);
		extern HAL_StatusTypeDef bsp_common_evt_set(uint32_t flag);
		extern HAL_StatusTypeDef bsp_common_evt_clear(uint32_t flag);
		extern uint32_t bsp_common_evt_get(uint32_t flag, uint8_t if_clear);
		
		

#ifdef __cplusplus
}
#endif

#endif /* __BSP_COMMON_H__ */
