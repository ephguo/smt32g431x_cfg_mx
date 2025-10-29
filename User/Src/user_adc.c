#include "user_adc.h"
#include "adc.h"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_adc_ex.h"
#include "svpwm_vf_test.h"
#include "tim.h"
#include "usart.h"
#include "user_params.h"

const float I_ADC_COEFF = 0.01933593f;
tx_data_frame_t tx_buf[2] = {
    {.head = {0x01, 0x01, 0x7f, 0x80}, .tail = {0x00, 0x00, 0x80, 0x7f}},
    {.head = {0x01, 0x01, 0x7f, 0x80}, .tail = {0x00, 0x00, 0x80, 0x7f}}
  };
tx_flags_t tx_flags = {.pending_buf = 0xff};
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  static int32_t ia_offset = 0, ib_offset = 0, ic_offset = 0;
  static uint8_t offset_flag = 0, offset_cnt = 0;
  int32_t ia_adc1_rk1 = 0, ib_adc2_rk1 = 0, ic_adc1_rk2 = 0;
  float ia_fb = 0, ib_fb = 0, ic_fb = 0;

  if (hadc->Instance == ADC1) {
    ia_adc1_rk1 = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
    ib_adc2_rk1 = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
    ic_adc1_rk2 = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);

    if (offset_flag == 1) {

      ia_fb = (ia_adc1_rk1 - ia_offset) * I_ADC_COEFF;
      ib_fb = (ib_adc2_rk1 - ib_offset) * I_ADC_COEFF;
      ic_fb = (ic_adc1_rk2 - ic_offset) * I_ADC_COEFF;

      set_vf_inputs();
      svpwm_vf_test_step();
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_abc.duty_a);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_abc.duty_b);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_abc.duty_c);

    } else {
      offset_cnt++;

      if (offset_cnt >= 8) {

        ia_offset += ia_adc1_rk1;
        ib_offset += ib_adc2_rk1;
        ic_offset += ic_adc1_rk2;

      }

      if (offset_cnt == 15) {

        ia_offset >>= 3;
        ib_offset >>= 3;
        ic_offset >>= 3;
        offset_flag = 1;

      }
    }
    
    tx_buf[tx_flags.tx_buf_idx].tx_data[0] = ia_fb;
    tx_buf[tx_flags.tx_buf_idx].tx_data[1] = ib_fb;
    tx_buf[tx_flags.tx_buf_idx].tx_data[2] = ic_fb;
    tx_buf[tx_flags.tx_buf_idx].tx_data[3] = duty_abc.duty_a;
    tx_buf[tx_flags.tx_buf_idx].tx_data[4] = duty_abc.duty_b;

    if (tx_flags.tx_busy == 0) {
      tx_flags.tx_busy = 1;
      HAL_UART_Transmit_DMA(&huart3, (uint8_t *)&tx_buf[tx_flags.tx_buf_idx],
                            sizeof(tx_data_frame_t));
    } else {
      tx_flags.pending_buf = tx_flags.tx_buf_idx;
    }

    tx_flags.tx_buf_idx ^= 1;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart3) {
    tx_flags.tx_busy = 0;
    if (tx_flags.pending_buf != 0xff) {
      tx_flags.pending_buf = 0xff;
      HAL_UART_Transmit_DMA(&huart3, (uint8_t *)&tx_buf[tx_flags.tx_buf_idx],
                            sizeof(tx_data_frame_t));
    }
  }
}
