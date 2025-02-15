#include "mprintf.h"

#include "stm32wlxx_ll_bus.h"
#include "stm32wlxx_ll_rcc.h"
#include "stm32wlxx_ll_system.h"
#include "stm32wlxx_ll_pwr.h"
#include "stm32wlxx_ll_gpio.h"
#include "stm32wlxx_ll_lpuart.h"
#include "stm32wlxx_ll_ipcc.h"
#include "stm32wlxx_ll_utils.h"
#include "stm32wlxx.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define CPU2_INITIALISED 0xAA
#define CPU2_NOT_INITIALISED 0xBE
volatile uint8_t *cpu2InitDone = (uint8_t *)0x2000FFFF;

volatile uint32_t *cycle_count = (uint32_t *)0x2000FFF0;


static void UART_init(void);
static void sysclk_init(void);


#define BUFFER_SIZE 0x1000

extern void* memmove_orig(void *destination, const void *source, size_t num);
extern void* memmove_(void *destination, const void *source, size_t num);

static void init_IPCC(void);
static inline void enable_cycle_count(void);
static inline uint32_t get_cycle_count(void);
static inline uint32_t get_LSU_count(void);


extern uint32_t _vector_table_offset;

int main(void)
{
  SCB->VTOR = (uint32_t)(&_vector_table_offset);  // set the vector table offset

  *cpu2InitDone = CPU2_NOT_INITIALISED;
  sysclk_init();
  enable_cycle_count();

  LL_PWR_EnableBootC2();

  while (*cpu2InitDone != CPU2_INITIALISED);

  while (1)
  {
    // wait for IPCC trigger
    if(LL_C2_IPCC_IsActiveFlag_CHx(IPCC, LL_IPCC_CHANNEL_1) == LL_IPCC_CHANNEL_1){
      *cycle_count = (volatile uint32_t)get_cycle_count;
      LL_C1_IPCC_ClearFlag_CHx(IPCC, LL_IPCC_CHANNEL_1);
      __NOP();
      __NOP();
    }
  }
}

static void init_IPCC(void)
{
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_IPCC);
}

static inline void enable_cycle_count(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->LSUCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk | DWT_CTRL_LSUEVTENA_Msk;
}

static inline uint32_t get_cycle_count(void)
{
  return DWT->CYCCNT;
}

static inline uint32_t get_LSU_count(void)
{
  return (DWT->LSUCNT) & 0xFF;
}




/******* Communication Functions *******/

int32_t putchar_(char c)
{
  // loop while the LPUART_TDR register is full
  while(LL_LPUART_IsActiveFlag_TXE_TXFNF(LPUART1) != 1);
  // once the LPUART_TDR register is empty, fill it with char c
  LL_LPUART_TransmitData8(LPUART1, (uint8_t)c);
  return (c);
}

static void sysclk_init(void)
{
  //set up to run off the 48MHz MSI clock
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  }

  // Configure the main internal regulator output voltage
  // set voltage scale to range 1, the high performance mode
  // this sets the internal main regulator to 1.2V and SYSCLK can be up to 64MHz
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while(LL_PWR_IsActiveFlag_VOS() == 1); // delay until VOS flag is 0

  // enable a wider range of MSI clock frequencies
  LL_RCC_MSI_EnableRangeSelection();

  /* Insure MSIRDY bit is set before writing MSIRANGE value */
  while (LL_RCC_MSI_IsReady() == 0U)
  {}

  /* Set MSIRANGE default value */
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_11);


  // delay until MSI is ready
  while (LL_RCC_MSI_IsReady() == 0U)
  {}

  // delay until MSI is system clock
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
  {}

  // update the global variable SystemCoreClock
  SystemCoreClockUpdate();

  // configure 1ms systick for easy delays
  LL_RCC_ClocksTypeDef clk_struct;
  LL_RCC_GetSystemClocksFreq(&clk_struct);
  LL_Init1msTick(clk_struct.HCLK1_Frequency);
}

static void UART_init(void)
{
  // enable the UART GPIO port clock
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

  // set the LPUART clock source to the peripheral clock
  LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);

  // enable clock for LPUART
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);

  // configure GPIO pins for LPUART1 communication
  // TX Pin is PA2, RX Pin is PA3
  LL_GPIO_InitTypeDef GPIO_InitStruct = {
  .Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3,
  .Mode = LL_GPIO_MODE_ALTERNATE,
  .Pull = LL_GPIO_PULL_NO,
  .Speed = LL_GPIO_SPEED_FREQ_MEDIUM,
  .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
  .Alternate = LL_GPIO_AF_8
};
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // configure the LPUART to transmit with the following settings:
  // baud = 115200, data bits = 8, stop bits = 1, parity bits = 0
  LL_LPUART_InitTypeDef LPUART_InitStruct = {
      .PrescalerValue = LL_LPUART_PRESCALER_DIV1,
      .BaudRate = 115200,
      .DataWidth = LL_LPUART_DATAWIDTH_8B,
      .StopBits = LL_LPUART_STOPBITS_1,
      .Parity = LL_LPUART_PARITY_NONE,
      .TransferDirection = LL_LPUART_DIRECTION_TX_RX,
      .HardwareFlowControl = LL_LPUART_HWCONTROL_NONE
  };
  LL_LPUART_Init(LPUART1, &LPUART_InitStruct);
  LL_LPUART_Enable(LPUART1);

  // wait for the LPUART module to send an idle frame and finish initialization
  while(!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || !(LL_LPUART_IsActiveFlag_REACK(LPUART1)));
}


