// Copyright 2015-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The LL layer for UART register operations.
// Note that most of the register operations in this layer are non-atomic operations.


#pragma once
#include "esp_attr.h"
#include "soc/uart_periph.h"
#include "soc/uart_struct.h"
#include "hal/uart_types.h"
#include "hal/hal_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// The default fifo depth
#define UART_LL_FIFO_DEF_LEN  (SOC_UART_FIFO_LEN)
// Get UART hardware instance with giving uart num
#define UART_LL_GET_HW(num) (((num) == 0) ? (&UART0) : (((num) == 1) ? (&UART1) : (&UART2)))

// The timeout calibration factor when using ref_tick
#define UART_LL_TOUT_REF_FACTOR_DEFAULT (8)

#define UART_LL_MIN_WAKEUP_THRESH (2)
#define UART_LL_INTR_MASK         (0x7ffff) //All interrupt mask

// Define UART interrupts
typedef enum {
    UART_INTR_RXFIFO_FULL      = (0x1<<0),
    UART_INTR_TXFIFO_EMPTY     = (0x1<<1),
    UART_INTR_PARITY_ERR       = (0x1<<2),
    UART_INTR_FRAM_ERR         = (0x1<<3),
    UART_INTR_RXFIFO_OVF       = (0x1<<4),
    UART_INTR_DSR_CHG          = (0x1<<5),
    UART_INTR_CTS_CHG          = (0x1<<6),
    UART_INTR_BRK_DET          = (0x1<<7),
    UART_INTR_RXFIFO_TOUT      = (0x1<<8),
    UART_INTR_SW_XON           = (0x1<<9),
    UART_INTR_SW_XOFF          = (0x1<<10),
    UART_INTR_GLITCH_DET       = (0x1<<11),
    UART_INTR_TX_BRK_DONE      = (0x1<<12),
    UART_INTR_TX_BRK_IDLE      = (0x1<<13),
    UART_INTR_TX_DONE          = (0x1<<14),
    UART_INTR_RS485_PARITY_ERR = (0x1<<15),
    UART_INTR_RS485_FRM_ERR    = (0x1<<16),
    UART_INTR_RS485_CLASH      = (0x1<<17),
    UART_INTR_CMD_CHAR_DET     = (0x1<<18),
} uart_intr_t;

/**
 * @brief  Set the UART source clock.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  source_clk The UART source clock. The source clock can be APB clock or REF_TICK.
 *                    If the source clock is REF_TICK, the UART can still work when the APB changes.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_sclk(uart_dev_t *hw, uart_sclk_t source_clk)
{
    hw->conf0.tick_ref_always_on = (source_clk == UART_SCLK_APB) ? 1 : 0;
}

/**
 * @brief  Get the UART source clock type.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  source_clk The pointer to accept the UART source clock type.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_get_sclk(uart_dev_t *hw, uart_sclk_t* source_clk)
{
    *source_clk = hw->conf0.tick_ref_always_on ? UART_SCLK_APB : UART_SCLK_REF_TICK;
}

/**
 * @brief  Get the UART source clock frequency.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return Current source clock frequency
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_sclk_freq(uart_dev_t *hw)
{
    return (hw->conf0.tick_ref_always_on) ? APB_CLK_FREQ : REF_CLK_FREQ;
}

/**
 * @brief  Configure the baud-rate.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  baud The baud-rate to be set. When the source clock is APB, the max baud-rate is `UART_LL_BITRATE_MAX`

 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_set_baudrate(uart_dev_t *hw, uint32_t baud)
{
    uint32_t sclk_freq, clk_div;

    sclk_freq = uart_ll_get_sclk_freq(hw);
    clk_div = ((sclk_freq) << 4) / baud;
    // The baud-rate configuration register is divided into
    // an integer part and a fractional part.
    hw->clk_div.div_int = clk_div >> 4;
    hw->clk_div.div_frag = clk_div &  0xf;
}

/**
 * @brief  Get the current baud-rate.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The current baudrate
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_baudrate(uart_dev_t *hw)
{
    uint32_t sclk_freq = uart_ll_get_sclk_freq(hw);
    typeof(hw->clk_div) div_reg;
    div_reg.val = hw->clk_div.val;
    return ((sclk_freq << 4)) / ((div_reg.div_int << 4) | div_reg.div_frag);
}

/**
 * @brief  Enable the UART interrupt based on the given mask.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  mask The bitmap of the interrupts need to be enabled.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_ena_intr_mask(uart_dev_t *hw, uint32_t mask)
{
    hw->int_ena.val |= mask;
}

/**
 * @brief  Disable the UART interrupt based on the given mask.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  mask The bitmap of the interrupts need to be disabled.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_disable_intr_mask(uart_dev_t *hw, uint32_t mask)
{
    hw->int_ena.val &= (~mask);
}

/**
 * @brief  Get the UART raw interrupt status.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The UART interrupt status.
 */
static inline uint32_t uart_ll_get_intraw_mask(uart_dev_t *hw)
{
    return hw->int_raw.val;
}

/**
 * @brief  Get the UART interrupt status.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The UART interrupt status.
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_intsts_mask(uart_dev_t *hw)
{
    return hw->int_st.val;
}

/**
 * @brief  Clear the UART interrupt status based on the given mask.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  mask The bitmap of the interrupts need to be cleared.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_clr_intsts_mask(uart_dev_t *hw, uint32_t mask)
{
    hw->int_clr.val = mask;
}

/**
 * @brief  Get status of enabled interrupt.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return Interrupt enabled value
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_intr_ena_status(uart_dev_t *hw)
{
    return hw->int_ena.val;
}

/**
 * @brief  Read the UART rxfifo.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  buf The data buffer. The buffer size should be large than 128 byts.
 * @param  rd_len The data length needs to be read.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_read_rxfifo(uart_dev_t *hw, uint8_t *buf, uint32_t rd_len)
{
    //Get the UART APB fifo addr. Read fifo, we use APB address
    uint32_t fifo_addr = (hw == &UART0) ? UART_FIFO_REG(0) : (hw == &UART1) ? UART_FIFO_REG(1) : UART_FIFO_REG(2);
    for(uint32_t i = 0; i < rd_len; i++) {
        buf[i] = READ_PERI_REG(fifo_addr);
#ifdef CONFIG_COMPILER_OPTIMIZATION_PERF
        __asm__ __volatile__("nop");
#endif
    }
}

/**
 * @brief  Write byte to the UART txfifo.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  buf The data buffer.
 * @param  wr_len The data length needs to be writen.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_write_txfifo(uart_dev_t *hw, const uint8_t *buf, uint32_t wr_len)
{
    //Get the UART AHB fifo addr, Write fifo, we use AHB address
    uint32_t fifo_addr = (hw == &UART0) ? UART_FIFO_AHB_REG(0) : (hw == &UART1) ? UART_FIFO_AHB_REG(1) : UART_FIFO_AHB_REG(2);
    for(uint32_t i = 0; i < wr_len; i++) {
        WRITE_PERI_REG(fifo_addr, buf[i]);
    }
}

/**
 * @brief  Reset the UART hw rxfifo.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_rxfifo_rst(uart_dev_t *hw)
{
    //Hardware issue: we can not use `rxfifo_rst` to reset the hw rxfifo.
    uint16_t fifo_cnt;
    typeof(hw->mem_rx_status) rxmem_sta;
    //Get the UART APB fifo addr
    uint32_t fifo_addr = (hw == &UART0) ? UART_FIFO_REG(0) : (hw == &UART1) ? UART_FIFO_REG(1) : UART_FIFO_REG(2);
    do {
        fifo_cnt = HAL_FORCE_READ_U32_REG_FIELD(hw->status, rxfifo_cnt);
        rxmem_sta.val = hw->mem_rx_status.val;
        if(fifo_cnt != 0 ||  (rxmem_sta.rd_addr != rxmem_sta.wr_addr)) {
            READ_PERI_REG(fifo_addr);
        } else {
            break;
        }
    } while(1);
}

/**
 * @brief  Reset the UART hw txfifo.
 *
 * Note:   Due to hardware issue, reset UART1's txfifo will also reset UART2's txfifo.
 *         So reserve this function for UART1 and UART2. Please do DPORT reset for UART and its memory at chip startup
 *         to ensure the TX FIFO is reset correctly at the beginning.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_txfifo_rst(uart_dev_t *hw)
{
    if (hw == &UART0) {
        hw->conf0.txfifo_rst = 1;
        hw->conf0.txfifo_rst = 0;
    }
}

/**
 * @brief  Get the length of readable data in UART rxfifo.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The readable data length in rxfifo.
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_rxfifo_len(uart_dev_t *hw)
{
    uint32_t fifo_cnt = HAL_FORCE_READ_U32_REG_FIELD(hw->status, rxfifo_cnt);
    typeof(hw->mem_rx_status) rx_status;
    rx_status.val = hw->mem_rx_status.val;
    uint32_t len = 0;

    // When using DPort to read fifo, fifo_cnt is not credible, we need to calculate the real cnt based on the fifo read and write pointer.
    // When using AHB to read FIFO, we can use fifo_cnt to indicate the data length in fifo.
    if (rx_status.wr_addr > rx_status.rd_addr) {
        len = rx_status.wr_addr - rx_status.rd_addr;
    } else if (rx_status.wr_addr < rx_status.rd_addr) {
        len = (rx_status.wr_addr + 128) - rx_status.rd_addr;
    } else {
        len = fifo_cnt > 0 ? 128 : 0;
    }

    return len;
}

/**
 * @brief  Get the writable data length of UART txfifo.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The data length of txfifo can be written.
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_txfifo_len(uart_dev_t *hw)
{
    return UART_LL_FIFO_DEF_LEN - HAL_FORCE_READ_U32_REG_FIELD(hw->status, txfifo_cnt);
}

/**
 * @brief  Configure the UART stop bit.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  stop_bit The stop bit number to be set.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_stop_bits(uart_dev_t *hw, uart_stop_bits_t stop_bit)
{
    //workaround for hardware issue, when UART stop bit set as 2-bit mode.
    if(stop_bit == UART_STOP_BITS_2)  {
        hw->rs485_conf.dl1_en = 1;
        hw->conf0.stop_bit_num = 0x1;
    } else {
        hw->rs485_conf.dl1_en = 0;
        hw->conf0.stop_bit_num = stop_bit;
    }
}

/**
 * @brief  Get the configuration of the UART stop bit.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  stop_bit The pointer to accept the stop bit configuration
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_get_stop_bits(uart_dev_t *hw, uart_stop_bits_t *stop_bit)
{
    //workaround for hardware issue, when UART stop bit set as 2-bit mode.
    if(hw->rs485_conf.dl1_en == 1 && hw->conf0.stop_bit_num == 0x1) {
        *stop_bit = UART_STOP_BITS_2;
    } else {
        *stop_bit = (uart_stop_bits_t)hw->conf0.stop_bit_num;
    }
}

/**
 * @brief  Configure the UART parity check mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  parity_mode The parity check mode to be set.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_parity(uart_dev_t *hw, uart_parity_t parity_mode)
{
    if(parity_mode != UART_PARITY_DISABLE) {
        hw->conf0.parity = parity_mode & 0x1;
    }
    hw->conf0.parity_en = (parity_mode >> 1) & 0x1;
}

/**
 * @brief  Get the UART parity check mode configuration.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  parity_mode The pointer to accept the parity check mode configuration.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_get_parity(uart_dev_t *hw, uart_parity_t *parity_mode)
{
    if(hw->conf0.parity_en) {
        *parity_mode = (uart_parity_t)(0X2 | hw->conf0.parity);
    } else {
        *parity_mode = UART_PARITY_DISABLE;
    }
}

/**
 * @brief  Set the UART rxfifo full threshold value. When the data in rxfifo is more than the threshold value,
 *         it will produce rxfifo_full_int_raw interrupt.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  full_thrhd The full threshold value of the rxfifo. `full_thrhd` should be less than `UART_LL_FIFO_DEF_LEN`.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_rxfifo_full_thr(uart_dev_t *hw, uint16_t full_thrhd)
{
    hw->conf1.rxfifo_full_thrhd = full_thrhd;
}

/**
 * @brief  Set the txfifo empty threshold. when the data length in txfifo is less than threshold value,
 *         it will produce txfifo_empty_int_raw interrupt.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  empty_thrhd The empty threshold of txfifo.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_txfifo_empty_thr(uart_dev_t *hw, uint16_t empty_thrhd)
{
    hw->conf1.txfifo_empty_thrhd = empty_thrhd;
}

/**
 * @brief  Set the UART rx-idle threshold value. when receiver takes more time than rx_idle_thrhd to receive a byte data,
 *         it will produce frame end signal for uhci to stop receiving data.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  rx_idle_thr The rx-idle threshold to be set.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_rx_idle_thr(uart_dev_t *hw, uint32_t rx_idle_thr)
{
    hw->idle_conf.rx_idle_thrhd = rx_idle_thr;
}

/**
 * @brief  Configure the duration time between transfers.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  idle_num the duration time between transfers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_tx_idle_num(uart_dev_t *hw, uint32_t idle_num)
{
    hw->idle_conf.tx_idle_num = idle_num;
}

/**
 * @brief  Configure the transmiter to send break chars.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  break_num The number of the break chars need to be send.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_tx_break(uart_dev_t *hw, uint32_t break_num)
{
    if(break_num > 0) {
        HAL_FORCE_MODIFY_U32_REG_FIELD(hw->idle_conf, tx_brk_num, break_num);
        hw->conf0.txd_brk = 1;
    } else {
        hw->conf0.txd_brk = 0;
    }
}

/**
 * @brief  Configure the UART hardware flow control.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  flow_ctrl The hw flow control configuration.
 * @param  rx_thrs The rx flow control signal will be active if the data length in rxfifo is more than this value.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_hw_flow_ctrl(uart_dev_t *hw, uart_hw_flowcontrol_t flow_ctrl, uint32_t rx_thrs)
{
    //only when UART_HW_FLOWCTRL_RTS is set , will the rx_thresh value be set.
    if(flow_ctrl & UART_HW_FLOWCTRL_RTS) {
        hw->conf1.rx_flow_thrhd = rx_thrs;
        hw->conf1.rx_flow_en = 1;
    } else {
        hw->conf1.rx_flow_en = 0;
    }
    if(flow_ctrl & UART_HW_FLOWCTRL_CTS) {
        hw->conf0.tx_flow_en = 1;
    } else {
        hw->conf0.tx_flow_en = 0;
    }
}

/**
 * @brief  Configure the hardware flow control.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  flow_ctrl A pointer to accept the hw flow control configuration.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_get_hw_flow_ctrl(uart_dev_t *hw, uart_hw_flowcontrol_t *flow_ctrl)
{
    *flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    if(hw->conf1.rx_flow_en) {
        *flow_ctrl = (uart_hw_flowcontrol_t)(*flow_ctrl | UART_HW_FLOWCTRL_RTS);
    }
    if(hw->conf0.tx_flow_en) {
        *flow_ctrl  = (uart_hw_flowcontrol_t)(*flow_ctrl | UART_HW_FLOWCTRL_CTS);
    }
}

/**
 * @brief  Configure the software flow control.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  flow_ctrl The UART sofware flow control settings.
 * @param  sw_flow_ctrl_en Set true to enable software flow control, otherwise set it false.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_sw_flow_ctrl(uart_dev_t *hw, uart_sw_flowctrl_t *flow_ctrl, bool sw_flow_ctrl_en)
{
    if(sw_flow_ctrl_en) {
        hw->flow_conf.xonoff_del = 1;
        hw->flow_conf.sw_flow_con_en = 1;
        HAL_FORCE_MODIFY_U32_REG_FIELD(hw->swfc_conf, xon_threshold, flow_ctrl->xon_thrd);
        HAL_FORCE_MODIFY_U32_REG_FIELD(hw->swfc_conf, xoff_threshold, flow_ctrl->xoff_thrd);
        HAL_FORCE_MODIFY_U32_REG_FIELD(hw->swfc_conf, xon_char, flow_ctrl->xon_char);
        HAL_FORCE_MODIFY_U32_REG_FIELD(hw->swfc_conf, xoff_char, flow_ctrl->xoff_char);
    } else {
        hw->flow_conf.sw_flow_con_en = 0;
        hw->flow_conf.xonoff_del = 0;
    }
}

/**
 * @brief  Configure the AT cmd char. When the receiver receives a continuous AT cmd char, it will produce at_cmd_char_det interrupt.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  cmd_char The AT cmd char configuration.The configuration member is:
 *         - cmd_char The AT cmd character
 *         - char_num The number of received AT cmd char must be equal to or greater than this value
 *         - gap_tout The interval between each AT cmd char, when the duration is less than this value, it will not take this data as AT cmd char
 *         - pre_idle The idle time before the first AT cmd char, when the duration is less than this value, it will not take the previous data as the last AT cmd char
 *         - post_idle The idle time after the last AT cmd char, when the duration is less than this value, it will not take this data as the first AT cmd char
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_at_cmd_char(uart_dev_t *hw, uart_at_cmd_t *cmd_char)
{
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->at_cmd_char, data, cmd_char->cmd_char);
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->at_cmd_char, char_num, cmd_char->char_num);
    hw->at_cmd_postcnt.post_idle_num = cmd_char->post_idle;
    hw->at_cmd_precnt.pre_idle_num = cmd_char->pre_idle;
    hw->at_cmd_gaptout.rx_gap_tout = cmd_char->gap_tout;
}

/**
 * @brief  Set the UART data bit mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  data_bit The data bit mode to be set.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_data_bit_num(uart_dev_t *hw, uart_word_length_t data_bit)
{
    hw->conf0.bit_num = data_bit;
}

/**
 * @brief  Set the rts active level.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  level The rts active level, 0 or 1.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_rts_active_level(uart_dev_t *hw, int level)
{
    hw->conf0.sw_rts = level & 0x1;
}

/**
 * @brief  Set the dtr active level.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  level The dtr active level, 0 or 1.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_dtr_active_level(uart_dev_t *hw, int level)
{
    hw->conf0.sw_dtr = level & 0x1;
}

/**
 * @brief  Set the UART wakeup threshold.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  wakeup_thrd The wakeup threshold value to be set. When the input rx edge changes more than this value,
 *         the UART will active from light sleeping mode.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_wakeup_thrd(uart_dev_t *hw, uint32_t wakeup_thrd)
{
    hw->sleep_conf.active_threshold = wakeup_thrd  - UART_LL_MIN_WAKEUP_THRESH;
}

/**
 * @brief  Configure the UART work in normal mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode_normal(uart_dev_t *hw)
{
    hw->rs485_conf.en = 0;
    hw->rs485_conf.tx_rx_en = 0;
    hw->rs485_conf.rx_busy_tx_en = 0;
    hw->conf0.irda_en = 0;
}

/**
 * @brief  Configure the UART work in rs485_app_ctrl mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode_rs485_app_ctrl(uart_dev_t *hw)
{
    // Application software control, remove echo
    hw->rs485_conf.rx_busy_tx_en = 1;
    hw->conf0.irda_en = 0;
    hw->conf0.sw_rts = 0;
    hw->conf0.irda_en = 0;
    hw->rs485_conf.en = 1;
}

/**
 * @brief  Configure the UART work in rs485_half_duplex mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode_rs485_half_duplex(uart_dev_t *hw)
{
    // Enable receiver, sw_rts = 1  generates low level on RTS pin
    hw->conf0.sw_rts = 1;
    // Must be set to 0 to automatically remove echo
    hw->rs485_conf.tx_rx_en = 0;
    // This is to void collision
    hw->rs485_conf.rx_busy_tx_en = 1;
    hw->conf0.irda_en = 0;
    hw->rs485_conf.en = 1;
}

/**
 * @brief  Configure the UART work in collision_detect mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode_collision_detect(uart_dev_t *hw)
{
    hw->conf0.irda_en = 0;
    // Transmitters output signal loop back to the receivers input signal
    hw->rs485_conf.tx_rx_en = 1 ;
    // Transmitter should send data when the receiver is busy
    hw->rs485_conf.rx_busy_tx_en = 1;
    hw->conf0.sw_rts = 0;
    hw->rs485_conf.en = 1;
}

/**
 * @brief  Configure the UART work in irda mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode_irda(uart_dev_t *hw)
{
    hw->rs485_conf.en = 0;
    hw->rs485_conf.tx_rx_en = 0;
    hw->rs485_conf.rx_busy_tx_en = 0;
    hw->conf0.sw_rts = 0;
    hw->conf0.irda_en = 1;
}

/**
 * @brief  Set uart mode.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  mode The UART mode to be set.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_mode(uart_dev_t *hw, uart_mode_t mode)
{
    switch (mode) {
        default:
        case UART_MODE_UART:
            uart_ll_set_mode_normal(hw);
            break;
        case UART_MODE_RS485_COLLISION_DETECT:
            uart_ll_set_mode_collision_detect(hw);
            break;
        case UART_MODE_RS485_APP_CTRL:
            uart_ll_set_mode_rs485_app_ctrl(hw);
            break;
        case UART_MODE_RS485_HALF_DUPLEX:
            uart_ll_set_mode_rs485_half_duplex(hw);
            break;
        case UART_MODE_IRDA:
            uart_ll_set_mode_irda(hw);
            break;
    }
}

/**
 * @brief  Get the UART AT cmd char configuration.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  cmd_char The Pointer to accept value of UART AT cmd char.
 * @param  char_num Pointer to accept the repeat number of UART AT cmd char.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_get_at_cmd_char(uart_dev_t *hw, uint8_t *cmd_char, uint8_t *char_num)
{
    *cmd_char = HAL_FORCE_READ_U32_REG_FIELD(hw->at_cmd_char, data);
    *char_num = HAL_FORCE_READ_U32_REG_FIELD(hw->at_cmd_char, char_num);
}

/**
 * @brief  Get the UART wakeup threshold value.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return The UART wakeup threshold value.
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_wakeup_thrd(uart_dev_t *hw)
{
    return hw->sleep_conf.active_threshold + UART_LL_MIN_WAKEUP_THRESH;
}

/**
 * @brief  Get the UART data bit configuration.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  data_bit The pointer to accept the UART data bit configuration.
 *
 * @return The bit mode.
 */
FORCE_INLINE_ATTR void uart_ll_get_data_bit_num(uart_dev_t *hw, uart_word_length_t *data_bit)
{
    *data_bit = (uart_word_length_t)hw->conf0.bit_num;
}

/**
 * @brief  Check if the UART sending state machine is in the IDLE state.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return True if the state machine is in the IDLE state, otherwise false is returned.
 */
FORCE_INLINE_ATTR IRAM_ATTR bool uart_ll_is_tx_idle(uart_dev_t *hw)
{
    typeof(hw->status) status;
    status.val = hw->status.val;
    return ((status.txfifo_cnt == 0) && (status.st_utx_out == 0));
}

/**
 * @brief  Check if the UART rts flow control is enabled.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return True if hw rts flow control is enabled, otherwise false is returned.
 */
FORCE_INLINE_ATTR bool uart_ll_is_hw_rts_en(uart_dev_t *hw)
{
    return hw->conf1.rx_flow_en;
}

/**
 * @brief  Check if the UART cts flow control is enabled.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return True if hw cts flow control is enabled, otherwise false is returned.
 */
FORCE_INLINE_ATTR bool uart_ll_is_hw_cts_en(uart_dev_t *hw)
{
    return hw->conf0.tx_flow_en;
}

/**
 * @brief Configure TX signal loop back to RX module, just for the testing purposes
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  loop_back_en Set ture to enable the loop back function, else set it false.
 *
 * @return None
 */
FORCE_INLINE_ATTR void uart_ll_set_loop_back(uart_dev_t *hw, bool loop_back_en)
{
    hw->conf0.loopback = loop_back_en;
}

/**
 * @brief  Inverse the UART signal with the given mask.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  inv_mask The UART signal bitmap needs to be inversed.
 *         Use the ORred mask of `uart_signal_inv_t`;
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_inverse_signal(uart_dev_t *hw, uint32_t inv_mask)
{
    typeof(hw->conf0) conf0_reg;
    conf0_reg.val = hw->conf0.val;
    conf0_reg.irda_tx_inv = (inv_mask & UART_SIGNAL_IRDA_TX_INV) ? 1 : 0;
    conf0_reg.irda_rx_inv = (inv_mask & UART_SIGNAL_IRDA_RX_INV) ? 1 : 0;
    conf0_reg.rxd_inv = (inv_mask & UART_SIGNAL_RXD_INV) ? 1 : 0;
    conf0_reg.cts_inv = (inv_mask & UART_SIGNAL_CTS_INV) ? 1 : 0;
    conf0_reg.dsr_inv = (inv_mask & UART_SIGNAL_DSR_INV) ? 1 : 0;
    conf0_reg.txd_inv = (inv_mask & UART_SIGNAL_TXD_INV) ? 1 : 0;
    conf0_reg.rts_inv = (inv_mask & UART_SIGNAL_RTS_INV) ? 1 : 0;
    conf0_reg.dtr_inv = (inv_mask & UART_SIGNAL_DTR_INV) ? 1 : 0;
    hw->conf0.val = conf0_reg.val;
}

/**
 * @brief  Configure the timeout value for receiver receiving a byte, and enable rx timeout function.
 *
 * @param  hw Beginning address of the peripheral registers.
 * @param  tout_thr The timeout value as a bit time. The rx timeout function will be disabled if `tout_thr == 0`.
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_set_rx_tout(uart_dev_t *hw, uint16_t tout_thr)
{
    if (hw->conf0.tick_ref_always_on == 0) {
        //Hardware issue workaround: when using ref_tick, the rx timeout threshold needs increase to 10 times.
        //T_ref = T_apb * APB_CLK/(REF_TICK << CLKDIV_FRAG_BIT_WIDTH)
        tout_thr = tout_thr * UART_LL_TOUT_REF_FACTOR_DEFAULT;
    } else {
        //If APB_CLK is used: counting rate is BAUD tick rate / 8
        tout_thr = (tout_thr + 7) / 8;
    }
    if (tout_thr > 0) {
        hw->conf1.rx_tout_thrhd = tout_thr;
        hw->conf1.rx_tout_en = 1;
    } else {
        hw->conf1.rx_tout_en = 0;
    }
}

/**
 * @brief  Get the timeout value for receiver receiving a byte.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return tout_thr The timeout threshold value. If timeout feature is disabled returns 0.
 */
FORCE_INLINE_ATTR uint16_t uart_ll_get_rx_tout_thr(uart_dev_t *hw)
{
    uint16_t tout_thrd = 0;
    if (hw->conf1.rx_tout_en > 0) {
        if (hw->conf0.tick_ref_always_on == 0) {
            tout_thrd = (uint16_t)(hw->conf1.rx_tout_thrhd / UART_LL_TOUT_REF_FACTOR_DEFAULT);
        } else {
            tout_thrd = (uint16_t)(hw->conf1.rx_tout_thrhd  << 3);
        }
    }
    return tout_thrd;
}

/**
 * @brief  Get UART maximum timeout threshold.
 *
 * @param  hw Beginning address of the peripheral registers.
 *
 * @return maximum timeout threshold.
 */
FORCE_INLINE_ATTR uint16_t uart_ll_max_tout_thrd(uart_dev_t *hw)
{
    uint16_t tout_thrd = 0;
    if (hw->conf0.tick_ref_always_on == 0) {
        tout_thrd = (uint16_t)(UART_RX_TOUT_THRHD_V / UART_LL_TOUT_REF_FACTOR_DEFAULT);
    } else {
        tout_thrd = (uint16_t)(UART_RX_TOUT_THRHD_V  << 3);
    }
    return tout_thrd;
}

/**
 * @brief  Force UART xoff.
 *
 * @param  uart_num UART port number, the max port number is (UART_NUM_MAX -1).
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_force_xoff(uart_port_t uart_num)
{
    /* Note: Set `UART_FORCE_XOFF` can't stop new Tx request. */
    REG_SET_BIT(UART_FLOW_CONF_REG(uart_num), UART_FORCE_XOFF);
}

/**
 * @brief  Force UART xon.
 *
 * @param  uart_num UART port number, the max port number is (UART_NUM_MAX -1).
 *
 * @return None.
 */
FORCE_INLINE_ATTR void uart_ll_force_xon(uart_port_t uart_num)
{
    REG_CLR_BIT(UART_FLOW_CONF_REG(uart_num), UART_FORCE_XOFF);
    REG_SET_BIT(UART_FLOW_CONF_REG(uart_num), UART_FORCE_XON);
    REG_CLR_BIT(UART_FLOW_CONF_REG(uart_num), UART_FORCE_XON);
}

/**
 * @brief  Get UART final state machine status.
 *
 * @param  uart_num UART port number, the max port number is (UART_NUM_MAX -1).
 *
 * @return UART module FSM status.
 */
FORCE_INLINE_ATTR uint32_t uart_ll_get_fsm_status(uart_port_t uart_num)
{
    return REG_GET_FIELD(UART_STATUS_REG(uart_num), UART_ST_UTX_OUT);
}

#undef UART_LL_TOUT_REF_FACTOR_DEFAULT

#ifdef __cplusplus
}
#endif
