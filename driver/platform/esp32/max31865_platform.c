/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2026 Reptaide.
 *
 * Released under the terms of the MIT License.
 * See LICENSE in the root of this repository for the full license text.
 */

#include "max31865_platform.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

/**
 * @brief Questa è la funzione che viene chiamata dopo un evento di interrupt.
 *
 * @param[in] arg Puntatore generico contenente varie informazioni.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    max31865_t *device = (max31865_t *)arg;

    if (!device || !device->isr_handler)
        return;

    device->isr_handler(device, device->context);
}

/**
 * @brief Implementa la logica per leggere il buffer SPI tramite ESP32.
 *
 * @param[in]  handle               Dispositivo MAX31865.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[in]  length               Dimensione del buffer.
 * @param[out] data                 Buffer dei dati da ricevere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore durante la ricezione dei dati.
 */
static max31865_err_t spi_read_register(
    void *handle,
    const uint8_t reg,
    const size_t length,
    uint8_t *data
)
{
    // Verifica i parametri
    if (!handle || !data || length == 0)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Ottiene la struttura del dispositivo
    max31865_t *device = (max31865_t *)handle;

    // Definisce i buffer (reg: 1 byte, dati: N byte)
    uint8_t tx_data[1 + length];
    uint8_t rx_data[1 + length];

    // Copia i dati nel buffer di trasmissione
    tx_data[0] = reg;
    memset(&tx_data[1], 0, length);

    // Azzera tutti i campi della struttura
    spi_transaction_t trans = {0};

    trans.length = 8 * (1 + length);   // Specifica i bit da trasferire
    trans.rxlength = 8 * (1 + length); // Specifica i bit da ricevere
    trans.tx_buffer = tx_data;         // Assegna il buffer TX alla struttura
    trans.rx_buffer = rx_data;         // Assegna il buffer RX alla struttura

    status = spi_device_transmit((spi_device_handle_t)device->spi_device_handle, &trans);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Copia i dati ricevuti tranne il registro
    memcpy(data, &rx_data[1], length);

    return MAX31865_ERR_OK;
}

/**
 * @brief Implementa la logica per scrivere il buffer SPI tramite ESP32.
 *
 * @param[in] handle                Dispositivo MAX31865.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] length                Dimensione del buffer.
 * @param[in] data                  Buffer dei dati da inviare.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore durante la ricezione dei dati.
 */
static max31865_err_t spi_write_register(
    void *handle,
    const uint8_t reg,
    const size_t length,
    const uint8_t *data
)
{
    // Verifica i parametri
    if (!handle || !data || length == 0)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Ottiene la struttura del dispositivo
    max31865_t *device = (max31865_t *)handle;

    // Definisce il buffer (reg: 1 byte, dati: N byte)
    uint8_t tx_data[1 + length];

    // Copia i dati nel buffer di trasmissione
    tx_data[0] = reg;
    memcpy(&tx_data[1], data, length);

    // Azzera tutti i campi della struttura
    spi_transaction_t trans = {0};

    trans.length = 8 * (1 + length); // Specifica i bit da trasferire
    trans.tx_buffer = tx_data;       // Assegna il buffer alla struttura

    status = spi_device_transmit((spi_device_handle_t)device->spi_device_handle, &trans);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    return MAX31865_ERR_OK;
}

/**
 * @brief Blocca per un certo periodo di millisecondi (ms) l'esecuzione del codice.
 *
 * @param[in] ms Tempo di attesa.
 */
static void delay_ms(const uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static const max31865_platform_t max31865_platform = {
    .spi_read = spi_read_register,
    .spi_write = spi_write_register,
    .delay_ms = delay_ms,
};

max31865_err_t max31865_init_hal(
    max31865_t *device,
    spi_host_device_t bus_handle,
    const uint8_t spi_cs_pin,
    const uint32_t spi_clk_speed
)
{
    // Verifica i parametri
    if (!device || !bus_handle)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;
    spi_device_handle_t device_handle;

    // Configura l'handle SPI del dispositivo
    spi_device_interface_config_t spi_device_config = {
        .address_bits = 0,
        .clock_speed_hz = spi_clk_speed,
        .command_bits = 0,
        .mode = 1,
        .queue_size = 1,
        .spics_io_num = spi_cs_pin,
    };

    // Inizializza il dispositivo sul bus SPI
    status = spi_bus_add_device(bus_handle, &spi_device_config, &device_handle);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Inizializza i campi restanti del dispositivo
    device->spi_bus_handle = (void *)bus_handle;
    device->spi_device_handle = (void *)device_handle;
    device->spi_cs_pin = spi_cs_pin;
    device->spi_clk_speed = spi_clk_speed;
    device->reference_resistor = 0.0f;
    device->rtd_nominal_resistance = 0.0f;
    device->int_pin = GPIO_NUM_NC;
    device->context = NULL;
    device->isr_handler = NULL;
    device->platform = &max31865_platform;

    return MAX31865_ERR_OK;
}

max31865_err_t max31865_hal_setup_int(max31865_t *device, const uint8_t int_pin)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Configurazione del pin INT
    gpio_config_t int_pin_config = {
        .intr_type = GPIO_INTR_NEGEDGE,        // Imposta l'interrupt active-low
        .mode = GPIO_MODE_INPUT,               // Configura il pin come input
        .pin_bit_mask = (1ULL << int_pin),     // Valore del pin da configurare
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disabilita il pull-down interno sul pin
        .pull_up_en = GPIO_PULLUP_ENABLE,      // Abilita il pull-up interno sul pin
    };

    // Applica la configurazione al GPIO
    status = gpio_config(&int_pin_config);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Aggiunge l'ISR handler al GPIO
    status = gpio_isr_handler_add(device->int_pin, gpio_isr_handler, (void *)device);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Aggiorna i campi del dispositivo
    device->int_pin = int_pin;

    return MAX31865_ERR_OK;
}
