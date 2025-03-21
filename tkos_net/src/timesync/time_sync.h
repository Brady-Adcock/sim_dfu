/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __TIME_SYNC_H__
#define __TIME_SYNC_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrfx.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TS_SOC_OBSERVER_PRIO
#define TS_SOC_OBSERVER_PRIO 0
#endif

#ifndef TIME_SYNC_TIMER_MAX_VAL
#define TIME_SYNC_TIMER_MAX_VAL (16000)
#endif

#ifndef TIME_SYNC_RTC_MAX_VAL
#define TIME_SYNC_RTC_MAX_VAL   (0xFFFFFF)
#endif

#ifndef TIME_SYNC_DESYNC_TIMEOUT
#define TIME_SYNC_DESYNC_TIMEOUT 10000000 /* Timeout for desynchronization [us] */
#endif

#ifndef TIME_SYNC_EVT_HANDLER_IRQ_PRIORITY
#define TIME_SYNC_EVT_HANDLER_IRQ_PRIORITY 5 /* Priority of event handler */
#endif

/**@brief Convert timestamp (@ref ts_timestamp_get_ticks_u32, @ref ts_timestamp_get_ticks_u64) to microsecond
 *
 * @note Do not use a function as argument directly, as _timestamp is expanded twice.
 */
#define TIME_SYNC_TIMESTAMP_TO_USEC(_timestamp) NRFX_ROUNDED_DIV((_timestamp), 16)

/**@brief Convert timestamp (@ref ts_timestamp_get_ticks_u32, @ref ts_timestamp_get_ticks_u64)to tick for use in @ref ts_set_trigger.
 *
 * @details Tick resolution for @ref ts_set_trigger is given by @ref TIME_SYNC_TIMER_MAX_VAL.
 *
 * @note Do not use a function as argument directly, as _msec is expanded twice.
 *
 * @note The output is rounded to the nearest integer, which can be 0.
 */
#define TIME_SYNC_TIMESTAMP_TO_TICK(_timestamp) NRFX_ROUNDED_DIV(_timestamp, TIME_SYNC_TIMER_MAX_VAL)

/**@brief Convert millisecond to tick for use in @ref ts_set_trigger.
 *
 * @details Tick resolution for @ref ts_set_trigger is given by @ref TIME_SYNC_TIMER_MAX_VAL.
 *
 * @note Do not use a function as argument directly, as _msec is expanded twice.
 *
 * @note The output is rounded to the nearest integer, which can be 0.
 */
#define TIME_SYNC_MSEC_TO_TICK(_msec) NRFX_ROUNDED_DIV((_msec * NRFX_ROUNDED_DIV(16000000, TIME_SYNC_TIMER_MAX_VAL)), 1000)




#define TIME_SYNC_FREQ_AUTO ((uint32_t) 5)

typedef enum
{
    TS_EVT_SYNCHRONIZED,   /* First sync packet received from transmitter */
    TS_EVT_DESYNCHRONIZED, /* @ref TIME_SYNC_DESYNC_TIMEOUT microseconds passed without any sync packets reception from transmitter */
    TS_EVT_TRIGGERED,      /* Trigger event set by @ref ts_set_trigger */
    TS_EVT_TIMESTAMP,      /* Timestamp triggered by PPI */
} ts_evt_type_t;

typedef struct
{
    ts_evt_type_t type;

    union
    {
        struct
        {
            // uint32_t tick_start;
            uint32_t tick_target;
            uint32_t last_sync;
            uint16_t sync_packet_count;
            uint16_t used_packet_count;
        } triggered;
        uint64_t timestamp;
    } params;
} ts_evt_t;

typedef struct {
    int32_t timer_offset_ticks;         /* Offset between local timer and master timer */
    uint32_t delta_time_ticks;          /* Time since last sync packet */
    uint64_t current_timestamp_ticks;   /* Synch'd timestamp counter value */
    uint32_t peer_counter;              /* COUNTER value for freewheel timer overflow count */
} ipc_sync_packet_t;

typedef void (*ts_evt_handler_t)(const ts_evt_t* evt);

/**@brief Initialize time sync library
 *
 * @param[in] p_params Parameters
 *
 * @retval 0 if successful
 */
int ts_init(ts_evt_handler_t evt_handler);

typedef struct
{
    uint8_t rf_chn; 	/** RF Channel [0-80] */
    uint8_t rf_addr[5]; /** 5-byte RF address */
} ts_rf_config_t;

/**@brief Enable time sync library. This will enable reception of sync packets.
 *
 * @retval 0 if successful
 */
int ts_enable(const ts_rf_config_t* p_rf_config);

/**@brief Disable time sync library.
 *
 * @retval 0 if successful
 */
int ts_disable(void);

/**@brief Start sync packet transmission (become timing master).
 *
 * @note @ref ts_enable() must be called prior to calling this function
 * @note Expect some jitter depending on BLE activity.
 *
 * @param[in] sync_freq_hz Frequency of transmitted sync packets. Use @ref TIME_SYNC_FREQ_AUTO to select automatically.
 *
 * @retval 0 if successful
 */
int ts_tx_start(uint32_t sync_freq_hz);

/**@brief Stop sync packet transmission (become timing slave again).
 *
 * @retval 0 if successful
 */
int ts_tx_stop(void);

#if !defined(DPPI_PRESENT)
/**@brief Trigger PPI endpoint at given tick
 *
 * @details Time unit is given by @ref TIME_SYNC_TIMER_MAX_VAL.
 *          Conversion is as follows:
 *          Time in milliseconds = target_tick * 16000000 / (TIME_SYNC_TIMER_MAX_VAL * 1000)
 *
 * @note When @ref ts_timestamp_get_ticks_u64 or @ref ts_timestamp_get_ticks_u32 is used as a reference,
 *       use @ref TIME_SYNC_TIMESTAMP_TO_TICK to convert to time unit for this function.
 *
 * @note Time sync receivers will adjust their local time according to the timing transmitter.
 *       If a trigger is set before a receiver is in sync with the transmitter, the local receiver timebase can skip ahead of the trigger time,
 *       causing the trigger to never occur.
 *
 * @param[in] target_tick  Time that PPI endpoint should be triggered.
 * @param[in] ppi_endpoint PPI endpoint to trigger.
 *
 * @return 0 or error value
 */
int ts_set_trigger(uint32_t target_tick, uint32_t ppi_endpoint);
#else
/**@brief Trigger DPPI channel at given tick
 *
 * @details Time unit is given by @ref TIME_SYNC_TIMER_MAX_VAL.
 *          Conversion is as follows:
 *          Time in milliseconds = target_tick * 16000000 / (TIME_SYNC_TIMER_MAX_VAL * 1000)
 *
 * @note When @ref ts_timestamp_get_ticks_u64 or @ref ts_timestamp_get_ticks_u32 is used as a reference,
 *       use @ref TIME_SYNC_TIMESTAMP_TO_TICK to convert to time unit for this function.
 *
 * @note Time sync receivers will adjust their local time according to the timing transmitter.
 *       If a trigger is set before a receiver is in sync with the transmitter, the local receiver timebase can skip ahead of the trigger time,
 *       causing the trigger to never occur.
 *
 * @param[in] target_tick  Time that PPI endpoint should be triggered.
 * @param[in] dppi_channel DPPI channel to trigger.
 *
 * @return 0 or error value
 */
int ts_set_trigger(uint32_t target_tick, uint8_t dppi_channel);
#endif

#if !defined(DPPI_PRESENT)
/**@brief Configure timestamp triggering from PPI event endpoint
 *
 * @details When configured, this PPI Event Endpoint will capture current timestamp value and generate @ref TS_EVT_TIMESTAMP event.
 *
 * @note Only TIMER instances 3 and higher can be used (@ref ts_init_t), as these have the extra compare registers required.
 *
 * @param[in] ppi_event_endpoint Address of PPI event endpoint that should capture timestamp
 *
 * @return 0 or error value
 */
int ts_set_timestamp_trigger(uint32_t ppi_event_endpoint);
#else
/**@brief Configure timestamp triggering from DPPI channel
 *
 * @details When configured, this DPPI channel will capture current timestamp value and generate @ref TS_EVT_TIMESTAMP event.
 *
 * @param[in] dppi_channel Address of DPPI channel that should capture timestamp
 *
 * @return 0 or error value
 */
int ts_set_timestamp_trigger(uint8_t dppi_channel);
#endif

/**@brief Get timestamp value in 16 MHz ticks
 *
 * @note 32-bit variable overflows after ~268 seconds
 *
 * @note When @ref TIME_SYNC_TX_OFFSET_REALIGN_TIMEOUT is non-zero,
 *       this function should be called from context priority @ref TIME_SYNC_EVT_HANDLER_IRQ_PRIORITY or lower
 *
 * @param[in] ppi_chn PPI channel to use for timer capture. Channel is not used after function exits.
 *
 * @retval timestamp value [1 second/16 MHz]
 */
uint32_t ts_timestamp_get_ticks_u32(void);

/**@brief Get timestamp value in 16 MHz ticks
 *
 * @note Internal 32-bit counter overflows after (2^32 * @ref TIME_SYNC_TIMER_MAX_VAL) / 16 000 000 seconds.
 *
 * @note When @ref TIME_SYNC_TX_OFFSET_REALIGN_TIMEOUT is non-zero,
 *       this function should be called from context priority @ref TIME_SYNC_EVT_HANDLER_IRQ_PRIORITY or lower
 *
 * @param[in] ppi_chn PPI channel to use for timer capture. Channel is not used after function exits.
 *
 * @retval timestamp value [1 second/16 MHz]
 */
uint64_t ts_timestamp_get_ticks_u64(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIME_SYNC_H__ */
