//------------------------------------------------------------------------------
// main.c
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2022 homelith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "boards/pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/vreg.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"

#include "cb.h"

#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

#define FRAME_WIDTH             80
#define FRAME_HEIGHT            60
#define PAGE_NUM                10

#define UART0_TX_QUEUE_SIZE     1024

#define PIN_LED                 25
#define PIN_7SEG_A              6
#define PIN_7SEG_B              7
#define PIN_7SEG_C              9
#define PIN_7SEG_D              2
#define PIN_7SEG_E              3
#define PIN_7SEG_F              5
#define PIN_7SEG_G              4
#define PIN_7SEG_DP             8
#define PIN_PSW                 10

typedef struct QUEUE_tag{
	uint8_t *buf;
	uint16_t head;
	uint16_t tail;
	uint16_t size;
} QUEUE;

struct dvi_inst dvi0;
uint8_t uart0_tx_queue_buf[UART0_TX_QUEUE_SIZE];

bool queue_push(QUEUE *q, uint8_t data) {
	uint16_t next_head = (q->head == q->size - 1) ? 0 : q->head + 1;
	if (next_head != q->tail) {
		q->buf[q->head] = data;
		q->head = next_head;
		return true;
	} else {
		return false;
	}
}
bool queue_pop(QUEUE *q, uint8_t *data) {
	if (q->head != q->tail) {
		(*data) = q->buf[q->tail];
		q->tail = (q->tail == q->size - 1) ? 0 : q->tail + 1;
		return true;
	} else {
		(*data) = 0;
		return false;
	}
}
uint8_t queue_puts(QUEUE *q, uint8_t *str, uint16_t num, bool stop_on_null) {
	for (uint8_t i = 0; i < num; i ++) {
		if (stop_on_null && str[i] == '\0') {
			return i;
		}
		if (! queue_push(q, str[i])) {
			return i;
		}
	}
	return num;
}

uint8_t hex2char(uint8_t h) {
	if (h < 10) {
		return (uint8_t)(h + 48);
	} else if (h < 16) {
		return (uint8_t)(h + 87);
	} else {
		return '*';
	}
}
uint8_t char2hex(uint8_t c) {
	if (c < 48) {
		return 0;
	} else if (c < 58) {
		return (uint8_t)(c - 48);
	} else if (c < 65) {
		return 0;
	} else if (c < 71) {
		return (uint8_t)(c - 55);
	} else if (c < 97) {
		return 0;
	} else if (c < 103) {
		return (uint8_t)(c - 87);
	} else {
		return 0;
	}
}

void set_7seg(uint8_t num) {
	uint8_t digit;
	switch(num) {
		case  0: digit = 0xC0; break;
		case  1: digit = 0xF9; break;
		case  2: digit = 0xA4; break;
		case  3: digit = 0xB0; break;
		case  4: digit = 0x99; break;
		case  5: digit = 0x92; break;
		case  6: digit = 0x82; break;
		case  7: digit = 0xD8; break;
		case  8: digit = 0x80; break;
		case  9: digit = 0x90; break;
		default: digit = 0xFF; break;
	}
	gpio_put(PIN_7SEG_A, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_B, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_C, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_D, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_E, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_F, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_G, (bool)(digit & 0x01)); digit = digit >> 1;
	gpio_put(PIN_7SEG_DP, (bool)(digit & 0x01)); digit = digit >> 1;
}

void core1_main() {
	// core1 extracts pixel data from 'CPU0 -> CPU1' FIFO, encode it to tmds symbol,
	// and push tmds symbol to 'CPU1 -> PIO' FIFO
	dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
	while (queue_is_empty(&dvi0.q_colour_valid))
		__wfe();
	dvi_start(&dvi0);
	dvi_scanbuf_x8scale_main_16bpp(&dvi0);
}


int main() {
	// queues
	QUEUE uart0_tx_queue;
	uart0_tx_queue.buf  = uart0_tx_queue_buf;
	uart0_tx_queue.head = 0;
	uart0_tx_queue.tail = 0;
	uart0_tx_queue.size = UART0_TX_QUEUE_SIZE;

	// switch controls
	bool     sw_delayline[3] = {false, false, false};
	bool     sw = false;
	bool     sw_prev = false;
	bool     sw_rise = false;
	uint64_t sw_prev_tick = 0;

	// dvi controls
	uint16_t dvi_line_idx = 0;
	uint16_t dvi_page_idx = 0;

	// system global setting
	vreg_set_voltage(VREG_VSEL);
	sleep_ms(10);
	set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

	// setup LED and push button
	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);
	gpio_init(PIN_7SEG_A);
	gpio_init(PIN_7SEG_B);
	gpio_init(PIN_7SEG_C);
	gpio_init(PIN_7SEG_D);
	gpio_init(PIN_7SEG_E);
	gpio_init(PIN_7SEG_F);
	gpio_init(PIN_7SEG_G);
	gpio_init(PIN_7SEG_DP);
	gpio_set_dir(PIN_7SEG_A, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_B, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_C, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_D, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_E, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_F, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_G, GPIO_OUT);
	gpio_set_dir(PIN_7SEG_DP, GPIO_OUT);
	gpio_init(PIN_PSW);
	gpio_set_dir(PIN_PSW, GPIO_IN);
	gpio_pull_up(PIN_PSW);

	// set up serials
	stdio_init_all();

	// init bitbang DVI library
	dvi0.timing = &DVI_TIMING;
	dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

	// Core 1 will wait until it sees the first colour buffer, then start up the
	// DVI signalling.
	multicore_launch_core1(core1_main);

	// init 7seg
	gpio_put(PIN_7SEG_A, true);
	gpio_put(PIN_7SEG_B, true);
	gpio_put(PIN_7SEG_C, true);
	gpio_put(PIN_7SEG_D, true);
	gpio_put(PIN_7SEG_E, true);
	gpio_put(PIN_7SEG_F, true);
	gpio_put(PIN_7SEG_G, true);
	gpio_put(PIN_7SEG_DP, true);
	set_7seg((dvi_page_idx + 1) % PAGE_NUM);

	// indicate init completed
	for (uint8_t i = 0; i < 3; i ++) {
		gpio_put(PIN_LED, true);
		sleep_ms(200);
		gpio_put(PIN_LED, false);
		sleep_ms(200);
	}
	queue_puts(&uart0_tx_queue, (uint8_t*)"pico-disp-ident started\r\n", 256, true);

	while (true) {
		uint64_t curr_tick = time_us_64();

		//--------------------------------------------------------------
		// remove sw chattering and detect edge
		// 10ms sampling -> 3 stage consensus -> edge detection
		//--------------------------------------------------------------
		if (curr_tick - sw_prev_tick > 10000) {
			sw_prev_tick = curr_tick;
			sw_delayline[2] = sw_delayline[1];
			sw_delayline[1] = sw_delayline[0];
			sw_delayline[0] = !(gpio_get(PIN_PSW));
			if (sw_delayline[2] & sw_delayline[1] & sw_delayline[0]) {
				sw = true;
			}
			if (!(sw_delayline[2] | sw_delayline[1] | sw_delayline[0])) {
				sw = false;
			}
		}
		if (!(sw_prev) & sw) {
			sw_rise = true;
		} else {
			sw_rise = false;
		}
		sw_prev = sw;

		// increment dvi page
		if (sw_rise) {
			if (dvi_page_idx == PAGE_NUM-1) {
				dvi_page_idx = 0;
			} else {
				dvi_page_idx ++;
			}
			// change 7seg state
			set_7seg((dvi_page_idx + 1) % PAGE_NUM);
		}


		//--------------------------------------------------------------
		// tx queue rate limited flushing
		// (approx. 1 byte per 1 horizontal scan time == 9600 times/s)
		//--------------------------------------------------------------
		// pop from uart0_tx_queue and push to uart0
		if (uart_is_writable(uart0)) {
			uint8_t c;
			if (queue_pop(&uart0_tx_queue, &c)) {
				putchar(c);
			}
		}

		//--------------------------------------------------------------
		// feeding image line to DVI color linebuffer
		//--------------------------------------------------------------
		// push image buf to inter CPU0 -> CPU1 FIFO
		const uint16_t *scanline = &((const uint16_t*)cb)[(dvi_page_idx * FRAME_HEIGHT + dvi_line_idx) * FRAME_WIDTH];
		queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
		while (queue_try_remove_u32(&dvi0.q_colour_free, &scanline));

		// increment line index
		if (dvi_line_idx == (FRAME_HEIGHT - 1)) {
			dvi_line_idx = 0;
		} else {
			dvi_line_idx ++;
		}
	}
}
