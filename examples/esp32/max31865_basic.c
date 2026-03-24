// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "max31865_core.h"
#include "max31865_platform.h"


#define SPI_BUS SPI2_HOST
#define SPI_PIN_MISO 11             // MISO/SDO/DOUT
#define SPI_PIN_MOSI 10             // MOSI/SDI/DIN
#define SPI_PIN_CLK 14              // CLK
#define SPI_PIN_CS 13               // CS
#define SPI_CLK_SPEED_HZ 1000000    // 1 MHz


static const char *TAG = "main";

/**
 * @brief Funzione di supporto per visualizzare lo stato dei bit.
 *
 * @param[in] name Nome per identificare il registro nella console.
 * @param[in] value  Valore del registro da visualizzare.
 */
static void print_reg_8(const char *name, const uint8_t value)
{
    ESP_LOGI(
        TAG,
        "%s (D[7:0]): %u %u %u %u %u %u %u %u",
        name,
        (value >> 7) & 0x01,
        (value >> 6) & 0x01,
        (value >> 5) & 0x01,
        (value >> 4) & 0x01,
        (value >> 3) & 0x01,
        (value >> 2) & 0x01,
        (value >> 1) & 0x01,
        (value >> 0) & 0x01
    );
}

/**
 * @brief Configura e inizializza il bus SPI
 */
void spi_bus_init(void)
{
    // Configura il bus SPI
    spi_bus_config_t spi_bus_config = {
        .miso_io_num = SPI_PIN_MISO,
        .mosi_io_num = SPI_PIN_MOSI,
        .sclk_io_num = SPI_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    // Inizializza il bus SPI
    ESP_ERROR_CHECK(
        spi_bus_initialize(SPI_BUS, &spi_bus_config, SPI_DMA_CH_AUTO)
    );

    ESP_LOGI(TAG, "SPI bus initialized successfully");
}

/**
 * @brief Entrypoint
 */
void app_main(void)
{
    uint8_t config = 0;
    float low_threshold = 0.0f;
    float high_threshold = 0.0f;
    uint8_t fault_status = 0;

    // Handler del bus SPI
    spi_host_device_t spi_bus_handle = SPI_BUS;

    // Inizializza il bus SPI
    spi_bus_init();

    // Definisce e inizializza a zero il dispositivo
    max31865_t device = {0};

    // Inizializza l'interfaccia hardware
    max31865_init_hal(&device, spi_bus_handle, SPI_PIN_CS, SPI_CLK_SPEED_HZ);

    // Inizializza il core
    max31865_init_core(&device, 430.0f, 100.0f, 0, 0);

    // Imposta le soglie di temperatura in gradi Celsius
    max31865_set_celsius_threshold(&device, -100.0f, 100.0f);

    // Ottine lo stato della configurazione
    max31865_get_config(&device, &config);
    print_reg_8("CONFIG", config);

    // Ottine i valori della soglia di temperatura impostate
    max31865_get_celsius_threshold(&device, &low_threshold, &high_threshold);
    ESP_LOGI(TAG, "LOW/HIGH THRESHOLD: %f / %f", low_threshold, high_threshold);

    // Ottiene lo stato dei guasti
    max31865_get_fault_status(&device, &fault_status);
    print_reg_8("FAULT STATUS", fault_status);

    // Imposta il campionamento della temperatura da 60 a 50 Hz
    max31865_set_config(&device, 0b00000001, 0b00000001);

    // Imposta le soglie di temperatura in gradi Celsius
    max31865_set_celsius_threshold(&device, 20.0f, 30.0f);

    // Ottine lo stato della configurazione
    max31865_get_config(&device, &config);
    print_reg_8("CONFIG", config);

    // Ottine i valori della soglia di temperatura impostate
    max31865_get_celsius_threshold(&device, &low_threshold, &high_threshold);
    ESP_LOGI(TAG, "LOW/HIGH THRESHOLD: %f / %f", low_threshold, high_threshold);

    // Ottiene lo stato dei guasti
    max31865_get_fault_status(&device, &fault_status);
    print_reg_8("FAULT STATUS", fault_status);

    // Rimuove il dispositivo MAX31865 dal bus SPI
    ESP_ERROR_CHECK(
        spi_bus_remove_device(device.spi_device_handle)
    );

    ESP_LOGI(TAG, "MAX31865 device removed successfully");

    // Libera il bus SPI
    ESP_ERROR_CHECK(
        spi_bus_free(SPI_BUS)
    );

    ESP_LOGI(TAG, "SPI bus successfully freed");
}
