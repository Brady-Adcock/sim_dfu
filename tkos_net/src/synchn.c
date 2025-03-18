/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

// what I think we need for gpio stuff... 
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>
#include <zephyr/drivers/gpio.h>

// stuff we need for the dppi and ipc and egu
#include <nrfx_dppi.h>
#include <zephyr/ipc/ipc_service.h>
#include <hal/nrf_ipc.h>
#include <hal/nrf_egu.h>


#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

// timer specific stuff
#include "time_sync.h"

LOG_MODULE_DECLARE(hci_ipc, LOG_LEVEL_DBG);

static bool m_gpio_trigger_enabled;

static uint8_t dppi_channel_syncpin;

// static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
// static const struct gpio_dt_spec timestamp_button = GPIO_DT_SPEC_GET(DT_ALIAS(stampbutton), gpios);
// static const struct gpio_dt_spec syncpin = GPIO_DT_SPEC_GET(DT_ALIAS(syncled), gpios);
// static nrfx_gpiote_pin_t syncpin_absval;

// static struct gpio_callback button_cb_data;

static nrfx_gpiote_t nrfx_gpiote = NRFX_GPIOTE_INSTANCE(0); // gpiote instance for entire program.

static void ts_gpio_trigger_enable(void)
{
	uint64_t time_now_ticks;
	uint32_t time_now_msec;
	uint32_t time_target;
	int err;

	if (m_gpio_trigger_enabled) {
		return;
	}

	// Round up to nearest second to next 1000 ms to start toggling.
	// If the receiver has received a valid sync packet within this time, the GPIO toggling polarity will be the same.

	time_now_ticks = ts_timestamp_get_ticks_u64();
	time_now_msec = TIME_SYNC_TIMESTAMP_TO_USEC(time_now_ticks) / 1000;

	time_target = TIME_SYNC_MSEC_TO_TICK(time_now_msec) + (1000 * 2);
	time_target = (time_target / 1000) * 1000;

	err = ts_set_trigger(time_target, dppi_channel_syncpin);
	__ASSERT_NO_MSG(err == 0);

	// nrfx_gpiote_set_task_trigger(&nrfx_gpiote, syncpin_absval);

	m_gpio_trigger_enabled = true;
}

static void ts_gpio_trigger_disable(void)
{
	m_gpio_trigger_enabled = false;
}

static void ts_event_handler(const ts_evt_t* evt)
{
	switch (evt->type)
	{
		case TS_EVT_SYNCHRONIZED:
			// ts_gpio_trigger_enable();
			LOG_INF("TS_EVT_SYNCHRONIZED");
			break;
		case TS_EVT_DESYNCHRONIZED:
			// ts_gpio_trigger_disable();
			LOG_INF("TS_EVT_DESYNCHRONIZED");
			break;
		case TS_EVT_TRIGGERED:
			if (m_gpio_trigger_enabled)
			{
				uint32_t tick_target;
				int err;

				/* Small increments here are more sensitive to drift.
				 * That is, an update from the timing transmitter that causes a jump larger than the
				 * chosen increment, risk having a trigger target_tick that is in the past.
				 */
				tick_target = evt->params.triggered.tick_target + 2;

#if defined(DPPI_PRESENT)
				//err = ts_set_trigger(tick_target, dppi_channel_syncpin);
				err = ts_set_trigger(tick_target, 255);
				__ASSERT_NO_MSG(err == 0);
#else
				err = ts_set_trigger(tick_target, nrfx_gpiote_out_task_address_get(syncpin_absval));
				__ASSERT_NO_MSG(err == 0);
#endif

			}
			else
			{
				// Ensure pin is low when triggering is stopped
				// nrfx_gpiote_clr_task_trigger(&nrfx_gpiote, syncpin_absval);
			}
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
	}
}

static void configure_sync_timer(void)
{
	int err;

	LOG_INF("Configuring time sync timer");

	//err = ts_init(ts_event_handler);
	err = ts_init(NULL);
	__ASSERT_NO_MSG(err == 0);

	LOG_INF("Time sync timer initialized");

	ts_rf_config_t rf_config = {
		.rf_chn = 80,
		.rf_addr = { 0xDE, 0xAD, 0xBE, 0xEF, 0x19 }
	};

	err = ts_enable(&rf_config);

	LOG_INF("Time sync enabled");

	__ASSERT_NO_MSG(err == 0);
}

// static nrfx_gpiote_pin_t pin_absval_get(const struct gpio_dt_spec *gpio_spec)
// {
// 	if (gpio_spec->port == DEVICE_DT_GET(DT_NODELABEL(gpio0))) {
// 		return NRF_GPIO_PIN_MAP(0, gpio_spec->pin);
// 	}
// #if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
// 	if (gpio_spec->port == DEVICE_DT_GET(DT_NODELABEL(gpio1))) {
// 		return NRF_GPIO_PIN_MAP(1, gpio_spec->pin);
// 	}
// #endif

// 	__ASSERT(false, "Port could not be determined");
// 	return 0;
// }

// static void configure_debug_gpio(void)
// {
// 	nrfx_err_t nrfx_err;
// 	int err;

// 	LOG_INF("Configuring debug GPIO pin");

// 	nrfx_gpiote_output_config_t gpiote_cfg = {
// 		.drive = NRF_GPIO_PIN_S0S1,
// 		.input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
// 		.pull = NRF_GPIO_PIN_NOPULL,
// 	};

// 	nrfx_gpiote_task_config_t task_cfg = {
// 		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
// 		.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
// 	};

// 	syncpin_absval = pin_absval_get(&syncpin);

// 	err = gpio_pin_configure_dt(&syncpin, GPIO_OUTPUT_LOW | syncpin.dt_flags);
// 	__ASSERT_NO_MSG(err == 0);

// 	if (nrfx_gpiote_init_check(&nrfx_gpiote) != false) {
// 		nrfx_err = nrfx_gpiote_init(&nrfx_gpiote, 5);
// 		// __ASSERT_NO_MSG(nrfx_err == NRFX_SUCCESS);
// 		__ASSERT_NO_MSG(nrfx_err == NRFX_ERROR_ALREADY);
// 		LOG_INF("nrfx_gpiote_init() successful");
// 	} else {
// 		LOG_INF("nrfx_gpiote_init() already called");
// 	}

// 	nrfx_err = nrfx_gpiote_channel_alloc(&nrfx_gpiote, &task_cfg.task_ch);
// 	__ASSERT_NO_MSG(nrfx_err == NRFX_SUCCESS);
// 	LOG_INF("nrfx_gpiote_channel_alloc() successful");

// 	nrfx_err = nrfx_gpiote_output_configure(&nrfx_gpiote, syncpin_absval, &gpiote_cfg, &task_cfg);
// 	__ASSERT_NO_MSG(nrfx_err == NRFX_SUCCESS);
// 	LOG_INF("nrfx_gpiote_output_configure() successful");

// 	nrfx_gpiote_out_task_enable(&nrfx_gpiote, syncpin_absval);
// 	LOG_INF("nrfx_gpiote_out_task_enable() successful");

// 	LOG_INF("Debug GPIO pin configured");

// #if defined(DPPI_PRESENT)
// 	nrfx_err = nrfx_dppi_channel_alloc(&dppi_channel_syncpin);
// 	__ASSERT_NO_MSG(nrfx_err == NRFX_SUCCESS);

// 	nrf_gpiote_subscribe_set(
// 		NRF_GPIOTE, nrfx_gpiote_out_task_get(&nrfx_gpiote, syncpin_absval), dppi_channel_syncpin);

// 	nrf_dppi_channels_enable(NRF_DPPIC_NS, BIT(dppi_channel_syncpin));
// #endif

// 	LOG_INF("Debug GPIO pin DPPI channel setup complete and enabled");
// }

void toggle_master_synch_node() {
	static bool is_master_node = true;
	LOG_ERR("Master node toggle! is_master_node: %s", is_master_node ? "true" : "false");
	int err;

	if (is_master_node) {
			LOG_ERR("Starting sync beacon transmission");

			err = ts_tx_start(TIME_SYNC_FREQ_AUTO);
			__ASSERT_NO_MSG(err == 0);

			ts_gpio_trigger_enable();

			LOG_WRN("Sync beacon transmission set-up sucessful!");
			LOG_WRN("This device is now the master clock node.");
	} else {
		LOG_ERR("Stopping sync beacon transmission");
		m_gpio_trigger_enabled = false;

		err = ts_tx_stop();
		__ASSERT_NO_MSG(err == 0);

		LOG_WRN("Stopping sync beacon transmission sucessful!");
	}

	is_master_node = !is_master_node;
}


// void button_pressed(const struct device *dev, struct gpio_callback *cb,
// 		    uint32_t pins)
// {
// 	toggle_master_synch_node();
// }

void callback_setup(void)
{
	int err;

	// gpio_pin_configure_dt(&button, GPIO_INPUT);

	// err = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	// if (err != 0) {
	// 	LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
	// 		err, button.port->name, button.pin);
	// 	return;
	// }

	// gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	// gpio_add_callback(button.port, &button_cb_data);

	// LOG_INF("Button configured on %s pin %d", button.port->name, button.pin);
	// LOG_INF("Press the button to start the synch beacon transmissions.");

	// configure_debug_gpio();
	configure_sync_timer();

	return;
}

K_THREAD_DEFINE(callback_tid, 2048, callback_setup, NULL, NULL, NULL, 14, 0, 100); 