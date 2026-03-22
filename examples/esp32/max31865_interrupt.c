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
#define INT_PIN 12                  // DRDY/RDY/INT


static const char *TAG = "main";
static TaskHandle_t max31865_task_handle = NULL;

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
 * @brief Questa funzione è responsabile della gestione dell'evento di interrupt.
 * Viene eseguita in un task differente dal main, in questo modo non viene
 * bloccata l'esecuzione dell'applicazione principale.
 *
 * @param[in] arg Puntatore al dispositivo MAX31865.
 */
static void max31865_task(void *arg)
{
    max31865_t *device = (max31865_t *)arg;

    float temperature = 0.0f;
    uint8_t fault_status = 0;

    for(;;)
    {
        // Attende la notifica dall'ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Ottiene il valore della temperatura in gradi Celsius
        max31865_read_celsius(device, &temperature, NULL);
        ESP_LOGI(TAG, "TEMPERATURE °C: %f", temperature);

        // Ottiene lo stato dei guasti
        max31865_get_fault_status(device, &fault_status);
        print_reg_8("FAULT STATUS", fault_status);

        // Ritardo di 100 ms per la lettura in console
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Questa funzione serve a notificare un task quando avviene un interrupt hardware.
 *
 * @param[in] context Contiene il puntatore al TaskHandle_t
 */
static void max31865_irq_callback(void *context)
{
    TaskHandle_t task_handle = (TaskHandle_t)context;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(task_handle, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void app_main(void)
{
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
    max31865_set_celsius_threshold(&device, 20.0f, 30.0f);

    // Installa nella IRAM il servizio di interrupt GPIO globale
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    // Crea il task per gestire l'evento del sensore
    xTaskCreate(max31865_task, "max31865_task", 4096, &device, 5, &max31865_task_handle);

    // Registra il callback IRQ
    max31865_register_irq_callback(
        &device,
        (max31865_irq_callback_t)max31865_irq_callback,
        (void *)max31865_task_handle
    );

    // Configura il pin di interrupt
    max31865_hal_setup_int(&device, INT_PIN);

    // Avvia in modo automatico le conversioni di temperatura
    max31865_start_conversion(&device);

    // Loop infinito, previene la chiusura del programma
    for(;;)
        vTaskDelay(pdMS_TO_TICKS(1000));
}