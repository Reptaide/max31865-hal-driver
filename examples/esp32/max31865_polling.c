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


void app_main(void)
{
    uint8_t config = 0;
    uint8_t is_ready = 0;
    uint8_t rtd_fault = 0;
    uint8_t fault_status = 0;
    float temperature = 0.0f;

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

    // Ottine lo stato della configurazione
    max31865_get_config(&device, &config);
    print_reg_8("CONFIG", config);

    // Avvia una singola conversione di temperatura
    max31865_start_single_conversion(&device);
    ESP_LOGI(TAG, "SINGLE CONVERSION STARTED");

    // Ottine lo stato della configurazione
    max31865_get_config(&device, &config);
    print_reg_8("CONFIG", config);

    for(;;)
    {
        // Stampa un placeholder mentre si attende la conversione della temperatura
        ESP_LOGI(TAG, "DUMMY");

        // Verifica quando la conversione termina. Non può essere utilizzata in modalità continua
        max31865_is_data_ready(&device, &is_ready);

        if (is_ready)
        {
            // Ottine lo stato della configurazione
            max31865_get_config(&device, &config);
            print_reg_8("CONFIG", config);

            // Ottiene il valore della temperatura in gradi Celsius
            max31865_read_celsius(&device, &temperature, &rtd_fault);
            ESP_LOGI(TAG, "RTD FAULT: %u, TEMPERATURE °C: %f", rtd_fault, temperature);

            // Ottiene lo stato di fault del dispositivo
            max31865_get_fault_status(&device, &fault_status);
            print_reg_8("FAULT STATUS", fault_status);

            // Termina il ciclo infinito
            break;
        }
    }

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
