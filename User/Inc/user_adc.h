#ifndef __USER_ADC_H__
#define __USER_ADC_H__

#include <stdint.h>

typedef struct {
  uint8_t head[4];
  float tx_data[5];
  uint8_t tail[4];
} tx_data_frame_t;
extern tx_data_frame_t tx_data_frame;

typedef struct {
  uint8_t tx_buf_idx;
  uint8_t tx_busy;
  uint8_t pending_buf;
} tx_flags_t;
extern tx_flags_t tx_flags;

#endif /* __USER_ADC_H__ */
