/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrfx_saadc.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

LOG_MODULE_REGISTER(nrf_apm, LOG_LEVEL_DBG);

#define SAADC_SAMPLE_INTERVAL_US 50 // one cannot trigger sampling more often than N_channels*(T_ACQ+T_CONV).
#define SAADC_BUFFER_SIZE 8000
#if NRF_SAADC_HAS_AIN_AS_PIN
#if defined(CONFIG_SOC_NRF54L15)
#define NRF_SAADC_INPUT_AIN0 NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1)
#define NRF_SAADC_INPUT_AIN1 NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1)
#define NRF_SAADC_INPUT_AIN2 NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1)
#define NRF_SAADC_INPUT_AIN3 NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1)
#define NRF_SAADC_INPUT_AIN4 NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1)
#define NRF_SAADC_INPUT_AIN5 NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1)
#define NRF_SAADC_INPUT_AIN6 NRF_PIN_PORT_TO_PIN_NUMBER(13U, 1)
#define NRF_SAADC_INPUT_AIN7 NRF_PIN_PORT_TO_PIN_NUMBER(14U, 1)
#define SAADC_INPUT_PIN1 NRF_SAADC_INPUT_AIN4                   // NRF_SAADC_INPUT_VDD for vdd direct
#define SAADC_INPUT_PIN2 NRF_SAADC_INPUT_AIN5
#else
BUILD_ASSERT(0, "Unsupported device family");
#endif
#else
#define SAADC_INPUT_PIN1 NRF_SAADC_INPUT_AIN0
#endif
static nrfx_saadc_channel_t saadc_channels[] =
    {
        NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN1, 0),
        NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN2, 1),
};
#if defined(CONFIG_SOC_NRF54L15)
#define TIMER_INSTANCE_NUMBER 22
#else
#define TIMER_INSTANCE_NUMBER 2
#endif
const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER);

static int16_t saadc_sample_buffer[2][SAADC_BUFFER_SIZE];
static uint32_t saadc_current_buffer = 0;

static void configure_timer(void)
{
    nrfx_err_t err;

    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(1000000);
    err = nrfx_timer_init(&timer_instance, &timer_config, NULL);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_timer_init error: %08x", err);
        return;
    }

    uint32_t timer_ticks = nrfx_timer_us_to_ticks(&timer_instance, SAADC_SAMPLE_INTERVAL_US);
    nrfx_timer_extended_compare(&timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
}

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
    nrfx_err_t err;
    switch (p_event->type)
    {
    case NRFX_SAADC_EVT_CALIBRATEDONE:
        LOG_INF("SAADC event: CALIBRATEDONE");
        err = nrfx_saadc_mode_trigger();
        NRFX_ASSERT(status == NRFX_SUCCESS);
        break;

    case NRFX_SAADC_EVT_READY:
        nrfx_timer_enable(&timer_instance);
        break;

    case NRFX_SAADC_EVT_BUF_REQ:
        err = nrfx_saadc_buffer_set(saadc_sample_buffer[(saadc_current_buffer++) % 2], SAADC_BUFFER_SIZE);
        // err = nrfx_saadc_buffer_set(saadc_sample_buffer[((saadc_current_buffer == 0 )? saadc_current_buffer++ : 0)], SAADC_BUFFER_SIZE);
        if (err != NRFX_SUCCESS)
        {
            LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
            return;
        }
        break;

    case NRFX_SAADC_EVT_DONE:
        // Note: For multiple channels, data is interleaved in the buffer
        // First sample from channel 0, then first sample from channel 1, etc.
        int64_t average0 = 0;
        int64_t average1 = 0;
        int16_t max = INT16_MIN;
        int16_t min = INT16_MAX;
        int16_t current_value;
        for (int i = 0; i < p_event->data.done.size; i++)
        {
            current_value = ((int16_t *)(p_event->data.done.p_buffer))[i];
            if (i % 2 == 0)
            {
                average1 += current_value;
            }
            else
            {
                average0 += current_value;
            }
            if (current_value > max)
            {
                max = current_value;
            }
            if (current_value < min)
            {
                min = current_value;
            }
        }
        average0 = average0 / ((p_event->data.done.size) / 2.0);
        average1 = average1 / ((p_event->data.done.size) / 2.0); // you can extend this to more chs and divide down
        LOG_INF("SAADC buffer at 0x%x filled with %d samples", (uint32_t)p_event->data.done.p_buffer, p_event->data.done.size);
        LOG_INF("SAMPLES: AVG0=0x%08x, AVG1=0x%08x, MIN=0x%x, MAX=0x%x", (int16_t)average0, (int16_t)average1, min, max);
        // est, NRF_SAADC_RESOLUTION_14BIT, RESULT = [V(P) – V(N) ] * GAIN/REFERENCE * 2(RESOLUTION - m)
        LOG_INF("V0: %d mV V1: %d mV", (int)(((900 * 4) * average0) / ((1 << 12))), (int)(((900 * 4) * average1) / ((1 << 12))));
        break;
    default:
        LOG_INF("Unhandled SAADC evt %d", p_event->type);
        break;
    }
}

static void configure_saadc(void)
{
    nrfx_err_t err;
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
                DT_IRQ(DT_NODELABEL(adc), priority),
                nrfx_isr, nrfx_saadc_irq_handler, 0);

    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_init error: %08x", err);
        return;
    }

#if defined(CONFIG_SOC_NRF54L15)
    saadc_channels[0].channel_config.gain = NRF_SAADC_GAIN1_4;
    saadc_channels[1].channel_config.gain = NRF_SAADC_GAIN1_4;
#else
    saadc_channels[0].channel_config.gain = NRF_SAADC_GAIN1_6;
#endif
    err = nrfx_saadc_channels_config(saadc_channels, 2);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_channels_config error: %08x", err);
        return;
    }
    uint32_t channels_mask = nrfx_saadc_channels_configured_get();
    nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    err = nrfx_saadc_advanced_mode_set(channels_mask,
                                       NRF_SAADC_RESOLUTION_12BIT,
                                       &saadc_adv_config,
                                       saadc_event_handler);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_advanced_mode_set error: %08x", err);
        return;
    }

    err = nrfx_saadc_buffer_set(saadc_sample_buffer[0], SAADC_BUFFER_SIZE);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
        return;
    }
    err = nrfx_saadc_buffer_set(saadc_sample_buffer[1], SAADC_BUFFER_SIZE);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
        return;
    }

    // calibration will nrfx_saadc_mode_trigger();
    err = nrfx_saadc_offset_calibrate(saadc_event_handler);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_saadc_offset_calibrate error: %08x", err);
        return;
    }
}

static void configure_ppi(void)
{
    nrfx_err_t err;
    uint8_t m_saadc_sample_ppi_channel;
    uint8_t m_saadc_start_ppi_channel;

    err = nrfx_gppi_channel_alloc(&m_saadc_sample_ppi_channel);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_gppi_channel_alloc error: %08x", err);
        return;
    }

    err = nrfx_gppi_channel_alloc(&m_saadc_start_ppi_channel);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("nrfx_gppi_channel_alloc error: %08x", err);
        return;
    }

    nrfx_gppi_channel_endpoints_setup(m_saadc_sample_ppi_channel,
                                      nrfx_timer_compare_event_address_get(&timer_instance, NRF_TIMER_CC_CHANNEL0),
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));

    nrfx_gppi_channel_endpoints_setup(m_saadc_start_ppi_channel,
                                      nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START));

    nrfx_gppi_channels_enable(BIT(m_saadc_sample_ppi_channel));
    nrfx_gppi_channels_enable(BIT(m_saadc_start_ppi_channel));
}

int main(void)
{
    configure_timer();
    configure_saadc();
    configure_ppi();
    k_sleep(K_FOREVER);
}
