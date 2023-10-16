/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file    bsp_log.c
* @brief   This file provides code for fulfill LOG functions
******************************************************************************
*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "bsp_log.h"
#include "bsp_common.h"

#include <stdarg.h>
#include <stdio.h>

/* USER CODE BEGIN 0 */
#if LOG_IN_RTOS
#include "tx_api.h"
#include "app_azure_rtos.h"

#define LOG_TXD_MSG_SIZE (120U)                  /*Make sure it is integer multiple of 4 */
#define LOG_TXD_MSG_MAX_NUM (4U)
#define LOG_UART_TXD_BUF_SIZE ((LOG_TXD_MSG_SIZE + sizeof(void *)) * LOG_TXD_MSG_MAX_NUM)

#define TX_LOG_TXD_STACK_SIZE (1024U)
#define TX_LOG_TXD_PRIO (TX_MAX_PRIORITIES - 5)
#define TX_LOG_TXD_PREEMPTION_THRESHOLD TX_LOG_TXD_PRIO

#define LOG_RXD_MSG_SIZE (112U)                  /*Make sure it is integer multiple of 4 */
#define LOG_RXD_MSG_MAX_NUM (4U)
#define LOG_UART_RXD_BUF_SIZE ((LOG_RXD_MSG_SIZE + sizeof(void *)) * LOG_RXD_MSG_MAX_NUM)

#define TX_LOG_RXD_STACK_SIZE (1024U)
#define TX_LOG_RXD_PRIO (TX_MAX_PRIORITIES - 6)
#define TX_LOG_RXD_PREEMPTION_THRESHOLD TX_LOG_RXD_PRIO


typedef  struct
{
  const uint8_t * msg;
  uint16_t byte_num;
}   LOG_INFO_Typedef;

#if defined  ( __ICCARM__ )
static char LOG_UART_TXD_BUF[LOG_UART_TXD_BUF_SIZE] @(0x30000000U) = {0};
static char LOG_UART_RXD_BUF[LOG_UART_RXD_BUF_SIZE] @(0x30000600U) = {0};
#else
//static char LOG_UART_TXD_BUF[LOG_UART_TXD_BUF_SIZE] __attribute__((at(0x30000000U), zero_init));
static char LOG_UART_TXD_BUF[LOG_UART_TXD_BUF_SIZE] __attribute__((section(".bss.ARM.__at_0x30000000")));
static char LOG_UART_RXD_BUF[LOG_UART_RXD_BUF_SIZE] __attribute__((section(".bss.ARM.__at_0x30000600")));
#endif


TX_BLOCK_POOL LOG_UART_TXD_MEMPOOL;
TX_BLOCK_POOL LOG_UART_RXD_MEMPOOL;


static uint8_t is_init = 0;

TX_THREAD log_txd_thread;
TX_QUEUE log_txd_queue;
TX_SEMAPHORE log_txd_lock_sem;

TX_THREAD log_rxd_thread;
TX_QUEUE log_rxd_queue;
TX_SEMAPHORE log_rxd_lock_sem;


extern volatile UINT _tx_thread_preempt_disable;
extern VOID _tx_thread_system_preempt_check(VOID);

void tx_log_txd_thread_entry(ULONG thread_input);
void tx_log_rxd_thread_entry(ULONG thread_input);


#else
#define LOG_UART_TXD_BUF_SIZE 128
#define LOG_UART_PORT huart3

char LOG_UART_TXD_BUF[LOG_UART_TXD_BUF_SIZE] = {0};
static uint8_t is_init = 0;
volatile uint8_t is_dbg_uart_tx_busy = 0;

#endif

/* USER CODE END 0 */


/* USER CODE BEGIN 1 */

#if LOG_IN_RTOS
__weak uint8_t log_init(void *memory_ptr)
{
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;
  CHAR *pointer;
  
  /*in case of reinit it*/
  if (is_init)
    return LOG_STATUS_FAIL;
  
  /*** log rxd part init ***/
  /* Allocate the stack for log thread */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer,
                       TX_LOG_RXD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /* Create tx log thread.  */
  if (tx_thread_create(&log_rxd_thread, (CHAR *)"tx_log_rxd_thread", tx_log_rxd_thread_entry, 0, pointer,
                       TX_LOG_RXD_STACK_SIZE, TX_LOG_RXD_PRIO, TX_LOG_RXD_PREEMPTION_THRESHOLD,
                       TX_NO_TIME_SLICE, TX_DONT_START) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /* Allocate the stack for log queue */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer,
                       LOG_RXD_MSG_MAX_NUM * sizeof(LOG_INFO_Typedef), TX_NO_WAIT) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  if (tx_queue_create(&log_rxd_queue, (CHAR *)"log_rxd_queue", sizeof(LOG_INFO_Typedef)/4, pointer, LOG_RXD_MSG_MAX_NUM * sizeof(LOG_INFO_Typedef)) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /*create semphore*/
  if (tx_semaphore_create(&log_rxd_lock_sem, (CHAR *)"log_rxd_lock_sem", 1) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  };
  
  /*create memory pool*/
  if (tx_block_pool_create(&LOG_UART_RXD_MEMPOOL, (CHAR *)"LOG UART RXD MEMPOOL", LOG_RXD_MSG_SIZE, LOG_UART_RXD_BUF, LOG_UART_RXD_BUF_SIZE) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /*** log txd part init ***/
  /* Allocate the stack for log thread */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer,
                       TX_LOG_TXD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /* Create tx log thread.  */
  if (tx_thread_create(&log_txd_thread, (CHAR *)"tx_log_txd_thread", tx_log_txd_thread_entry, 0, pointer,
                       TX_LOG_TXD_STACK_SIZE, TX_LOG_TXD_PRIO, TX_LOG_TXD_PREEMPTION_THRESHOLD,
                       TX_NO_TIME_SLICE, TX_DONT_START) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /* Allocate the stack for log queue */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer,
                       LOG_TXD_MSG_MAX_NUM * sizeof(LOG_INFO_Typedef), TX_NO_WAIT) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  if (tx_queue_create(&log_txd_queue, (CHAR *)"log_txd_queue", sizeof(LOG_INFO_Typedef)/4, pointer, LOG_TXD_MSG_MAX_NUM * sizeof(LOG_INFO_Typedef)) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  /*create semphore*/
  if (tx_semaphore_create(&log_txd_lock_sem, (CHAR *)"log_txd_lock_sem", 1) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  };
  
  /*create memory pool*/
  if (tx_block_pool_create(&LOG_UART_TXD_MEMPOOL, (CHAR *)"LOG UART TXD MEMPOOL", LOG_TXD_MSG_SIZE, LOG_UART_TXD_BUF, LOG_UART_TXD_BUF_SIZE) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  
  //  MX_USART3_UART_Init();
  is_init = 1;
  
  tx_thread_resume(&log_rxd_thread);
  tx_thread_resume(&log_txd_thread);
  
  return LOG_STATUS_OK;
}

void log_recycle(void)
{
  /*** log rxd part init ***/
  if (log_rxd_thread.tx_thread_id != TX_CLEAR_ID)
  {
    tx_thread_terminate(&log_rxd_thread);
    tx_thread_delete(&log_rxd_thread);
  }
  if (log_rxd_queue.tx_queue_id != TX_CLEAR_ID)
  {
    tx_queue_delete(&log_rxd_queue);
  }
  if (log_rxd_lock_sem.tx_semaphore_id != TX_CLEAR_ID)
  {
    tx_semaphore_delete(&log_rxd_lock_sem);
  }
  
  if (LOG_UART_RXD_MEMPOOL.tx_block_pool_id != TX_CLEAR_ID)
  {
    tx_block_pool_delete(&LOG_UART_RXD_MEMPOOL);
  }
  
  /*** log txd part init ***/
  if (log_txd_thread.tx_thread_id != TX_CLEAR_ID)
  {
    tx_thread_terminate(&log_txd_thread);
    tx_thread_delete(&log_txd_thread);
  }
  if (log_txd_queue.tx_queue_id != TX_CLEAR_ID)
  {
    tx_queue_delete(&log_txd_queue);
  }
  if (log_txd_lock_sem.tx_semaphore_id != TX_CLEAR_ID)
  {
    tx_semaphore_delete(&log_txd_lock_sem);
  }
  
  if (LOG_UART_TXD_MEMPOOL.tx_block_pool_id != TX_CLEAR_ID)
  {
    tx_block_pool_delete(&LOG_UART_TXD_MEMPOOL);
  }
}

__weak uint8_t log_port_lock(void)
{
  if (tx_semaphore_get(&log_txd_lock_sem, TX_WAIT_FOREVER) != TX_SUCCESS)
  {
    return LOG_STATUS_ERROR;
  }
  return LOG_STATUS_OK;
}

__weak uint8_t log_port_unlock(void)
{
  if (!log_txd_lock_sem.tx_semaphore_count)
  {
    if (tx_semaphore_put(&log_txd_lock_sem) != TX_SUCCESS)
    {
      return LOG_STATUS_ERROR;
    }
  }
  return LOG_STATUS_OK;
}

__weak uint8_t log_port_unlock_fromISR(void)
{
  uint8_t retval = LOG_STATUS_OK;
  
  /*Enter critical section*/
  TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
      _tx_thread_preempt_disable++;
  
  if (!log_txd_lock_sem.tx_semaphore_count)
  {
    if (tx_semaphore_put(&log_txd_lock_sem) != TX_SUCCESS)
    {
      retval = LOG_STATUS_FAIL;
    }
  }
  
  /*Exit from critical section*/
  _tx_thread_preempt_disable--;
  TX_RESTORE
    _tx_thread_system_preempt_check();
  
  return retval;
}

__weak uint8_t log_port_try_lock(void)
{
  if (tx_semaphore_get(&log_txd_lock_sem, 0) != TX_SUCCESS)
  {
    return LOG_STATUS_BUSY;
  }
  return LOG_STATUS_OK;
}

__weak uint8_t log_port_wait_unlock(uint32_t waitTick)
{
  if (tx_semaphore_get(&log_txd_lock_sem, waitTick) == TX_SUCCESS)
  {
    if (tx_semaphore_put(&log_txd_lock_sem) == TX_SUCCESS)
    {
      return LOG_STATUS_OK;
    }
  }
  return LOG_STATUS_FAIL;
}

/**
* @name log_printf
* @note can not call from ISR
**/
int log_printf(const char *format, ...)
{
  uint16_t num;
  va_list args;
  LOG_INFO_Typedef log_info;
  char *msg_ptr;
  uint8_t need_recycle = 1;
  /*make sure uart is init*/
  if (!is_init)
  {
    // #warning "before use log_printf, you should init it via log_init first" "before use log_printf, you should init it via log_init first"
    return 0;
  }
  
  /*try to allocate an memory*/
  if (tx_block_allocate(&LOG_UART_TXD_MEMPOOL, (VOID **)&msg_ptr, TX_NO_WAIT) != TX_SUCCESS)
  {
    return 0;
  }
  
  /*get args*/
  va_start(args, format);
  num = vsnprintf(msg_ptr, LOG_TXD_MSG_SIZE, format, args);
  va_end(args);
  
  /*save to queue*/
  if (num > 0)
  {
    log_info.byte_num = num;
    log_info.msg = (uint8_t *)msg_ptr;
    
    if (tx_queue_send(&log_txd_queue, &log_info, TX_NO_WAIT) == TX_SUCCESS)
    {
      need_recycle = 0;
    }
    else
    {
      num = 0;
    }
  }
  
  /*it means this block is useless, so recycle it */
  if (need_recycle)
  {
    tx_block_release((void *)msg_ptr);
  }
  
  return num;
}



/**
* @brief  Function implementing the tx_log_txd_thread_entry thread.
* @param  thread_input: Hardcoded to 0.
* @retval None
*/
void tx_log_txd_thread_entry(ULONG thread_input)
{
  LOG_INFO_Typedef log_info = {0};
  /* USER CODE BEGIN tx_app_thread_entry */
  
  for (;;)
  {
    if (tx_queue_receive(&log_txd_queue, &log_info, TX_WAIT_FOREVER) == TX_SUCCESS)
    {
      
      if (log_port_try_lock() == LOG_STATUS_OK && (log_info.msg != NULL)&&(log_info.byte_num > 0) )
      {
        
        if(HAL_UART_Transmit_DMA(&LOG_UART_PORT, log_info.msg, log_info.byte_num) == HAL_OK){
          log_port_wait_unlock(TX_WAIT_FOREVER);
        }else{          
          log_port_unlock();
        }
        
        tx_block_release((void *)log_info.msg);
      }
    }
  }
  /* USER CODE END tx_app_thread_entry */
}


/**
* @brief  Function implementing the tx_log_txd_thread_entry thread.
* @param  thread_input: Hardcoded to 0.
* @retval None
*/
void tx_log_rxd_thread_entry(ULONG thread_input)
{
  LOG_INFO_Typedef log_info = {0};
  /* USER CODE BEGIN tx_app_thread_entry */
  if(log_receive_with_idle(0) != HAL_OK){
    log_printf("log_rxd_thread will exit\r\n");
    tx_thread_terminate(&log_rxd_thread);
  }
  
  for (;;)
  {
    tx_thread_sleep(10);
    if (tx_queue_receive(&log_rxd_queue, &log_info, TX_WAIT_FOREVER) == TX_SUCCESS)
    {
      if ( (log_info.msg != NULL)&&(log_info.byte_num > 0) )
      {
        log_printf(">: %s\r\n", (char*)log_info.msg);
        if(tx_block_release((void *)log_info.msg) == TX_SUCCESS){
          if(bsp_common_evt_get(EVT_FLAG_LOG_RXD_PAUSED, 1) != 0x0000){
            if(log_receive_with_idle(0) != HAL_OK){
              log_printf("log_rxd_thread will exit\r\n");
              tx_thread_terminate(&log_rxd_thread);
            }
          }
        }
      }
    }
  }
  /* USER CODE END tx_app_thread_entry */
}

HAL_StatusTypeDef log_receive_with_idle(uint16_t wait_tick)
{
  HAL_StatusTypeDef retval = HAL_ERROR;
  uint8_t* mem_block;
  if(tx_block_allocate(&LOG_UART_RXD_MEMPOOL, (VOID**)&mem_block, wait_tick) == TX_SUCCESS ){
    retval = HAL_UARTEx_ReceiveToIdle_DMA(&LOG_UART_PORT, mem_block, LOG_RXD_MSG_SIZE - 1);
  }
  
  return retval;
}

HAL_StatusTypeDef log_save_receive(uint8_t* src, uint16_t length, uint16_t wait_tick)
{
  HAL_StatusTypeDef retval = HAL_ERROR;
  
  if((src != NULL)&&(length > 0)&&(length < LOG_RXD_MSG_SIZE - 1)){
    if(log_rxd_queue.tx_queue_available_storage > 0){
      LOG_INFO_Typedef info;
      info.msg = src;
      info.byte_num = length;
      
      if(tx_queue_send(&log_rxd_queue, &info, wait_tick) == TX_SUCCESS){
        retval = HAL_OK;
      }
    }
  }
  
  
  return retval;
}



#else
__weak uint8_t log_init(void)
{
  if (is_init)
    return LOG_STATUS_FAIL;
  
  MX_USART4_UART_Init();
  
  is_init = 1;
  return LOG_STATUS_OK;
}
__weak uint8_t log_port_lock(void)
{
  is_dbg_uart_tx_busy = 1;
  return LOG_STATUS_OK;
}
__weak uint8_t log_port_unlock(void)
{
  is_dbg_uart_tx_busy = 0;
  return LOG_STATUS_OK;
}
__weak uint8_t log_port_unlock_fromISR(void)
{
  return log_port_unlock();
}

__weak uint8_t log_port_try_lock(void)
{
  if (is_dbg_uart_tx_busy == 0)
  {
    is_dbg_uart_tx_busy = 1;
    return LOG_STATUS_OK;
  }
  return LOG_STATUS_FAIL;
}
__weak uint8_t log_port_wait_unlock(uint32_t waitTick)
{
  uint32_t checkpoint = 0;
  
  if (waitTick == 0)
    return LOG_STATUS_OK;
  
  checkpoint = HAL_GetTick() + waitTick;
  while (is_dbg_uart_tx_busy)
  {
    if (HAL_GetTick() >= checkpoint)
    {
      return LOG_STATUS_TIMEOUT;
    }
  }
  
  return LOG_STATUS_OK;
}

int log_printf(const char *format, ...)
{
  uint16_t num;
  va_list args;
  
  /*make sure uart is init*/
  if (!is_init)
  {
    // #warning "before use log_printf, you should init it via log_init first" "before use log_printf, you should init it via log_init first"
    return 0;
  }
  
  /*in case of busy*/
  if (log_port_try_lock() == LOG_STATUS_OK)
    return 0;
  
  /*get args*/
  va_start(args, format);
  num = vsnprintf(LOG_UART_TXD_BUF, LOG_UART_TXD_BUF_SIZE, format, args);
  va_end(args);
  
  /*tramsmit*/
  if (num > 0)
  {
    HAL_UART_Transmit_DMA(&LOG_UART_PORT, (uint8_t *)LOG_UART_TXD_BUF, num);
  }
  
  return num;
}

#endif

/* USER CODE END 1 */
