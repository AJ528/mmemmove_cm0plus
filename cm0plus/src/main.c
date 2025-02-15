#include "mprintf.h"
#include "greatest.h"

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
static void init_IPCC(void);

inline static uint32_t get_cycle_count(void);


#define BUFFER_SIZE 0x1000

extern void* memmove_orig(void *destination, const void *source, size_t num);
extern void* memmove_(void *destination, const void *source, size_t num);


TEST memmove_test(uint32_t data_len, uint32_t src_offset, uint32_t dest_offset, bool print_performance)
{

  uint8_t expected[BUFFER_SIZE] = {0};
  uint8_t actual[BUFFER_SIZE] = {0};

  if(((data_len + src_offset) > BUFFER_SIZE) || ((data_len + dest_offset) > BUFFER_SIZE)){
    FAIL();
  }


  uint32_t i;

  for(i = 0; i < data_len; i++){
    expected[src_offset + i] = i;
    actual[src_offset + i] = i;
  }

  // uint32_t memmove_orig_LSU_start = get_LSU_count();
  // uint32_t memmove_orig_start = get_cycle_count();
  memmove(&(expected[dest_offset]), &(expected[src_offset]), data_len);
  // uint32_t memmove_orig_stop = get_cycle_count();
  // uint32_t memmove_orig_LSU_stop = get_LSU_count();

  // uint32_t memmove_new_LSU_start = get_LSU_count();
  // uint32_t memmove_new_start = get_cycle_count();
  memmove_(&(actual[dest_offset]), &(actual[src_offset]), data_len);
  // uint32_t memmove_new_stop = get_cycle_count();
  //   uint32_t memmove_new_LSU_stop = get_LSU_count();



  // if(print_performance){
  //   uint32_t orig_cycle = memmove_orig_stop-memmove_orig_start;
  //   uint32_t new_cycle = memmove_new_stop-memmove_new_start;
  //   uint8_t orig_LSU = memmove_orig_LSU_stop-memmove_orig_LSU_start;
  //   uint8_t new_LSU = memmove_new_LSU_stop-memmove_new_LSU_start;
  //   printfln_("%-6u %-#10x %-#10x %-8u %-6u %-8u %-6u", data_len, src_offset, dest_offset, orig_cycle, orig_LSU, new_cycle, new_LSU);
  // }

  ASSERT_MEM_EQ(expected, actual, BUFFER_SIZE);

  PASS();
}

TEST memmove_iterate(uint32_t data_len_limit)
{
  uint32_t data_len;
  uint32_t src_offset;
  uint32_t dest_offset;
  uint32_t offset_limit;

  for(data_len = 0; data_len < data_len_limit; data_len++){
    offset_limit = data_len + 8;
    for(src_offset = 0; src_offset < offset_limit; src_offset++){
      for(dest_offset = 0; dest_offset < offset_limit; dest_offset++){
        CHECK_CALL(memmove_test(data_len, src_offset, dest_offset, false));
      }
    }
  }

  PASS();
}

TEST memmove_slide_dest(uint32_t data_len, uint32_t src_offset)
{
  uint32_t dest_offset;

  if((src_offset < data_len) || (src_offset + (2 * data_len)) > BUFFER_SIZE){
    FAIL();
  }

  CHECK_CALL(memmove_test(data_len, src_offset, 0, true));
  CHECK_CALL(memmove_test(data_len, src_offset, src_offset-data_len, true));

  CHECK_CALL(memmove_test(data_len, src_offset, src_offset - 4, true));
  CHECK_CALL(memmove_test(data_len, src_offset, src_offset - 1, true));
  CHECK_CALL(memmove_test(data_len, src_offset, src_offset, true));
  CHECK_CALL(memmove_test(data_len, src_offset, src_offset + 1, true));
  CHECK_CALL(memmove_test(data_len, src_offset, src_offset + 4, true));

  CHECK_CALL(memmove_test(data_len, src_offset, src_offset+data_len, true));
  CHECK_CALL(memmove_test(data_len, src_offset, BUFFER_SIZE-data_len, true));

  PASS();
}

// Add definitions that need to be in the test runner's main file.
// GREATEST_MAIN_DEFS();
extern uint32_t _vector_table_offset;

int main(void)
{
  SCB->VTOR = (uint32_t)(&_vector_table_offset);  // set the vector table offset

  *cpu2InitDone = CPU2_INITIALISED;
  sysclk_init();
  UART_init();

  println_("CPU2 LIVES!");

  uint32_t curr_cycle = get_cycle_count();

  printfln_("current cycle count is: %#x", curr_cycle);

  uint32_t new_cycle = get_cycle_count();

  printfln_("new cycle count is: %#x", new_cycle);
  printfln_("it took %#x cycles to print the first line", new_cycle-curr_cycle);


  // printfln_("%-6s %-10s %-10s %-8s %-6s %-8s %-6s", "d_len", "src_off", "dest_off", "o_cycle", "o_LSU", "n_cycle", "n_LSU");

  // GREATEST_MAIN_BEGIN();  // command-line options, initialization.

  // RUN_TESTp(memmove_test, 1, 0x81, 0x7E, true);
  // RUN_TESTp(memmove_test, 10, 0x81, 0x7E, true);
  // RUN_TESTp(memmove_test, 20, 0x81, 0x7E, true);
  // RUN_TESTp(memmove_test, 50, 0x81, 0x7E, true);

  // RUN_TESTp(memmove_test, 1, 0x81, 0x82, true);
  // RUN_TESTp(memmove_test, 10, 0x81, 0x82, true);
  // RUN_TESTp(memmove_test, 20, 0x81, 0x82, true);
  // RUN_TESTp(memmove_test, 50, 0x81, 0x82, true);

  // RUN_TESTp(memmove_test, 1, 0x81, 0x102, true);
  // RUN_TESTp(memmove_test, 10, 0x81, 0x102, true);
  // RUN_TESTp(memmove_test, 20, 0x81, 0x102, true);
  // RUN_TESTp(memmove_test, 50, 0x81, 0x102, true);

  // RUN_TESTp(memmove_slide_dest, 0x0f, 0x81);
  // RUN_TESTp(memmove_slide_dest, 0x10, 0x80);
  // RUN_TESTp(memmove_slide_dest, 0x22, 0x17f);
  // RUN_TESTp(memmove_slide_dest, 0x200, 0x400);

  // RUN_TEST1(memmove_iterate, 15);

  // GREATEST_MAIN_END();    // display results

  while (1)
  {
  	LL_mDelay(1000);
  }
}

static void init_IPCC(void)
{
  LL_C2_AHB3_GRP1_EnableClock(LL_C2_AHB3_GRP1_PERIPH_IPCC);
}

inline static uint32_t get_cycle_count(void)
{
  LL_C2_IPCC_SetFlag_CHx(IPCC, LL_IPCC_CHANNEL_1);
  while(LL_C2_IPCC_IsActiveFlag_CHx(IPCC, LL_IPCC_CHANNEL_1) == LL_IPCC_CHANNEL_1){}
  uint32_t current_cycle = *cycle_count;
  return current_cycle;
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
  LL_C2_AHB2_GRP1_EnableClock(LL_C2_AHB2_GRP1_PERIPH_GPIOA);

  // set the LPUART clock source to the peripheral clock
  LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);

  // enable clock for LPUART
  LL_C2_APB1_GRP2_EnableClock(LL_C2_APB1_GRP2_PERIPH_LPUART1);

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


