// SPDX-License-Identifier: MIT


#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max31865_platform.h"

/**
 * @brief Questa è la funzione che viene chiamata dopo un evento di interrupt.
 *
 * @param[in] arg Recupera il puntatore del dispositivo (passato al momento della registrazione).
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    max31865_t *device = (max31865_t *)arg;

    // Se il callback è registrato, lo invoca passandogli il contesto
    if (device->irq_callback)
        device->irq_callback(device->irq_context);
}

/**
 * @brief Questa funzione racchiude la logica per leggere dal bus SPI.
 *
 * @param[in] handle                Dispositivo MAX31865.
 * @param[in] reg                   Valore del registro da leggere.
 * @param[in] length                Specifica la dimensione del buffer.
 * @param[out] data                 Buffer con i dati da ricevere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore durante la ricezione dei dati.
 */
static max31865_err_t spi_read_register(void *handle, const uint8_t reg, const size_t length, uint8_t *data)
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

    trans.length = 8 * (1 + length);    // Specifica i bit da trasferire
    trans.rxlength = 8 * (1 + length);  // Specifica i bit da ricevere
    trans.tx_buffer = tx_data;          // Assegna il buffer TX alla struttura
    trans.rx_buffer = rx_data;          // Assegna il buffer RX alla struttura

    status = spi_device_transmit(
        (spi_device_handle_t)device->spi_device_handle,
        &trans
    );

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Copia i dati ricevuti tranne il registro
    memcpy(data, &rx_data[1], length);

    return MAX31865_ERR_OK;
}

/**
 * @brief Questa funzione racchiude la logica per scrivere sul bus SPI.
 *
 * @param[in] handle                Dispositivo MAX31865.
 * @param[in] reg                   Valore del registro da scrivere.
 * @param[in] length                Specifica la dimensione del buffer.
 * @param[in] data                  Buffer con i dati da inviare.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore durante l'invio dei dati.
 */
static max31865_err_t spi_write_register(void *handle, const uint8_t reg, const size_t length, const uint8_t *data)
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

    trans.length = 8 * (1 + length);    // Specifica i bit da trasferire
    trans.tx_buffer = tx_data;          // Assegna il buffer alla struttura

    status = spi_device_transmit(
        (spi_device_handle_t)device->spi_device_handle,
        &trans
    );

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    return MAX31865_ERR_OK;
}

/**
 * @brief Questa funzione attende in modo bloccante un certo periodo di tempo.
 *
 * @param[in] ms Valore del tempo di attesa.
 */
static void delay_ms(const uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}


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
    // Verifica il parametro
    if (device == NULL)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;
    spi_device_handle_t device_handle;

    // Configura l'handle SPI del sensore
    spi_device_interface_config_t spi_device_config = {
        .address_bits = 0,
        .clock_speed_hz = spi_clk_speed,
        .command_bits = 0,
        .mode = 1,
        .queue_size = 1,
        .spics_io_num = spi_cs_pin,
    };

    // Inizializza il sensore sul bus SPI
    status = spi_bus_add_device(
        bus_handle,
        &spi_device_config,
        &device_handle
    );

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Inizializza i campi della struttura
    // del sensore inerenti all'hardware
    device->spi_bus_handle = (void *)bus_handle;
    device->spi_device_handle = (void *)device_handle;
    device->spi_cs_pin = spi_cs_pin;
    device->spi_clk_speed = spi_clk_speed;
    device->reference_resistor = 0.0f;
    device->rtd_nominal_resistance = 0.0f;
    device->int_pin = -1;
    device->irq_context = NULL;
    device->irq_callback = NULL;
    device->platform = &max31865_platform;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_hal_setup_int(max31865_t *device, const uint8_t int_pin)
{
    // Verifica il parametro
    if (device == NULL)
        return MAX31865_ERR_INVALID_ARG;

    esp_err_t status = ESP_OK;

    // Assegna al dispositivo il valore del pin di interrupt
    device->int_pin = int_pin;

    // Configurazione del pin di interrupt
    gpio_config_t inta_pin_config = {
        .intr_type = GPIO_INTR_NEGEDGE,             // Imposta l'interrupt quando il segnale passa da high a low
        .mode = GPIO_MODE_INPUT,                    // Configura il pin come input
        .pin_bit_mask = (1ULL << device->int_pin),  // Specifica il GPIO da configurare in base alla maschera
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // Disabilita il pull-down interno sul GPIO
        .pull_up_en = GPIO_PULLUP_ENABLE,           // Abilita il pull-up interno sul GPIO
    };

    // Applica la configurazione al GPIO
    status = gpio_config(&inta_pin_config);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    // Aggiunge l'ISR handler al GPIO specificato
    status = gpio_isr_handler_add(device->int_pin, gpio_isr_handler, (void *)device);

    if (status != ESP_OK)
        return MAX31865_ERR_FAIL;

    return MAX31865_ERR_OK;
}
