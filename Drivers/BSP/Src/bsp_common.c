/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    bsp_common.c
 * @brief   This file provides shareable code for different bsp driver
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_common.h"
#include "bsp_log.h"
#include "main.h"
#include "usart.h"

/* Typedef ------------------------------------------------------------------*/

/* Variables ------------------------------------------------------------------*/
TX_EVENT_FLAGS_GROUP bsp_evt_group;
/* Functions  ------------------------------------------------------------------*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == LOG_UART_PORT.Instance)
  {
    log_port_unlock_fromISR();
  }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  uint32_t errcode = HAL_UART_GetError(huart);
  
  
  if(errcode != HAL_UART_ERROR_NONE){
    if(errcode == HAL_UART_ERROR_DMA){
      uint32_t dma_err = HAL_DMA_GetError(huart->hdmatx);
      if( dma_err != HAL_DMA_ERROR_NONE){
        if(dma_err == HAL_DMA_ERROR_TE){
          uint32_t te_flag = __HAL_DMA_GET_TE_FLAG_INDEX(huart->hdmatx);
          __HAL_DMA_CLEAR_FLAG(huart->hdmatx, te_flag);
        }
        HAL_UART_DMAStop(huart);
      }
    }else{
      __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF|UART_CLEAR_NEF|UART_CLEAR_OREF|UART_CLEAR_CMF|UART_CLEAR_LBDF|UART_CLEAR_TXFECF|UART_CLEAR_IDLEF);
    }
  }
  
  
  if (huart->Instance == LOG_UART_PORT.Instance)
  {
    
    log_port_unlock_fromISR();
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  
  HAL_UART_RxEventTypeTypeDef rx_evt;
  if (huart->Instance == LOG_UART_PORT.Instance)
  {
    rx_evt = HAL_UARTEx_GetRxEventType(huart);
    /* Log接受完成中断处理函数 . Normal mode, not Circular mode*/
    if(HAL_UART_RXEVENT_TC == rx_evt || HAL_UART_RXEVENT_IDLE == rx_evt){
      if((huart->pRxBuffPtr != NULL)&&(Size > 0)){
        huart->pRxBuffPtr[Size] = '\0';
        log_save_receive(huart->pRxBuffPtr, Size, 0);
        if(log_receive_with_idle(0) != HAL_OK){
          /* 暂停log接收， 并发送事件标志 */
          bsp_common_evt_set(EVT_FLAG_LOG_RXD_PAUSED);
        }
      }
    }
  }
}


HAL_StatusTypeDef bsp_common_init(void)
{
  HAL_StatusTypeDef retval = HAL_OK;
  static uint8_t is_init = 0;
  if(!is_init){
    if(tx_event_flags_create(&bsp_evt_group, "bsp_com_evt_group") == TX_SUCCESS){
      is_init = 1;
    }else{
      retval = HAL_ERROR;
    }
  }
  return retval;
}


HAL_StatusTypeDef bsp_common_evt_set(uint32_t flag)
{
  HAL_StatusTypeDef retval = HAL_ERROR;
  if((flag != 0x0000)&&(bsp_evt_group.tx_event_flags_group_id != TX_CLEAR_ID)){
    if(tx_event_flags_set(&bsp_evt_group, flag, TX_OR) == TX_SUCCESS){
      retval = HAL_OK;
    }
  }
  return retval;
}


HAL_StatusTypeDef bsp_common_evt_clear(uint32_t flag)
{
  HAL_StatusTypeDef retval = HAL_ERROR;
  if((flag != 0x0000)&&(bsp_evt_group.tx_event_flags_group_id != TX_CLEAR_ID)){
    uint32_t mask = ~flag;
    if(tx_event_flags_set(&bsp_evt_group, mask, TX_AND) == TX_SUCCESS){
      retval = HAL_OK;
    }
  }
  return retval;
}

uint32_t bsp_common_evt_get(uint32_t flag, uint8_t if_clear)
{
  uint32_t retval = 0x00;
  if((flag != 0x0000)&&(bsp_evt_group.tx_event_flags_group_id != TX_CLEAR_ID)){
    UINT get_option;
    ULONG actual_events;
    if(if_clear){
      get_option = TX_AND;
    }else{
      get_option = TX_AND_CLEAR;
    }
    if(tx_event_flags_get(&bsp_evt_group, flag, get_option, &actual_events,0) == TX_SUCCESS){
      retval = (uint32_t)actual_events;
    }
  }
  
  return retval;
}

/* bsp_common.c */
