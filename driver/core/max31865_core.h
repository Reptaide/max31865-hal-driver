// SPDX-License-Identifier: MIT


#ifndef MAX31865_H
#define MAX31865_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MAX31865_ERR_OK = 0,
    MAX31865_ERR_FAIL,
    MAX31865_ERR_INVALID_ARG,
} max31865_err_t;

typedef max31865_err_t (*max31865_read_register_t)(void *handle, const uint8_t reg, const size_t length, uint8_t *data);
typedef max31865_err_t (*max31865_write_register_t)(void *handle, const uint8_t reg, const size_t length, const uint8_t *data);
typedef void (*max31865_time_delay_t)(const uint32_t ms);
typedef void (*max31865_irq_callback_t)(void *context);

typedef struct
{
    max31865_read_register_t spi_read;
    max31865_write_register_t spi_write;
    max31865_time_delay_t delay_ms;
} max31865_platform_t;

typedef struct
{
    void *spi_bus_handle;                   // Handler del bus SPI
    void *spi_device_handle;                // Handler dispositivo SPI
    uint8_t spi_cs_pin;                     // Valore del pin CS utilizzato nel protocollo SPI
    uint32_t spi_clk_speed;                 // Valore della velocità di clock in Hz del dispositivo
    float reference_resistor;               // Valore resistenza di riferimento in Ohm
    float rtd_nominal_resistance;           // Resistenza nominale RTD a 0 °C in Ohm (PT100 = 100 Ohms, PT1000 = 1000 Ohms)
    int8_t int_pin;                         // Segnale di interrupt(-1 se non utilizzato)
    void *irq_context;                      // Contesto del segnale IRQ
    max31865_irq_callback_t irq_callback;   // Funzione di callback per l'evento IRQ
    const max31865_platform_t *platform;    // Contiene le funzioni che si interfacciano con l'hardware
} max31865_t;

/**
 * @brief Inizializza il core del dispositivo MAX31865.
 *
 * @param[in] device                    Dispositivo MAX31865.
 * @param[in] reference_resistor        Valore della resistenza di riferimento presente nella PCB.
 * @param[in] rtd_nominal_resistance    Valore della resistenza RTD (PT100: 100 Ohm, PT1000: 1000 Ohm).
 * @param[in] wire_number               Numero cavi della sonda (0: 2 o 4 cavi, 1: 3 cavi).
 * @param[in] filter_mode               Frequenza di campionamento (0: 60 Hz, 1: 50 Hz).
 * @retval MAX31865_ERR_OK              Successo.
 * @retval MAX31865_ERR_INVALID_ARG     Parametri non validi.
 */
max31865_err_t max31865_init_core(
    max31865_t *device,
    const float reference_resistor,
    const float rtd_nominal_resistance,
    const uint8_t wire_number,
    const uint8_t filter_mode
);

/**
 * @brief Legge il valore dei parametri dal registro di configurazione.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] value                 Stato dei bit della configurazione.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_get_config(max31865_t *device, uint8_t *value);

/**
 * @brief Imposta i parametri del registro di configurazione.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] bitmask               Maschera dei bit da modificare (0: Valore inalterato, 1: Valore da modificare).
 * @param[in] value                 Valore dei bit da impostare.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_set_config(max31865_t *device, const uint8_t bitmask, const uint8_t value);

/**
 * @brief COnverte il valore RTD in °C.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] rtd                   Valore RTD.
 * @param[out] celsius              Valore della temperatura in °C
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_rtd_to_celsius(max31865_t *device, const uint16_t rtd, float *celsius);

/**
 * @brief Converte il valore dei gradi Celsius in RTD.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] celsius               Valore della temperatura in °C.
 * @param[out] rtd                  Valore RTD.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_celsius_to_rtd(max31865_t *device, float celsius, uint16_t *rtd);

/**
 * @brief Restituisce il valore RTD e lo stato di fault.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] rtd                  Valore RTD.
 * @param[out] fault                Stato di fault, NULL se non richiesto.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_read_rtd(max31865_t *device, uint16_t *rtd, uint8_t *fault);

/**
 * @brief Restituisce i limiti delle soglie di temperatura impostate in RTD.
 * I parametri "lower" e "upper" non possono essere entrambi NULL
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] lower                Valore della soglia inferiore in RTD, NULL se non richiesto.
 * @param[out] upper                Valore della soglia superiore in RTD, NULL se non richiesto.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_get_rtd_threshold(max31865_t *device, uint16_t *lower, uint16_t *upper);

/**
 * @brief Imposta i limiti delle soglie di temperatura in RTD.
 * I parametri "lower" e "upper" non possono essere entrambi NULL
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] lower                 Valore della soglia inferiore in RTD.
 * @param[in] upper                 Valore della soglia superiore in RTD.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_set_rtd_threshold(max31865_t *device, uint16_t lower, uint16_t upper);

/**
 * @brief Restituisce il valore della temperatura in gradi Celsius e lo stato di fault.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] celsius              Valore della temperatura in °C.
 * @param[out] fault                Stato di fault, NULL se non richiesto.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_read_celsius(max31865_t *device, float *celsius, uint8_t *fault);

/**
 * @brief Restituisce i limiti delle soglie di temperatura impostate in gradi Celsius.
 * I parametri "lower" e "upper" non possono essere entrambi NULL
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] lower                Valore della soglia inferiore in °C, NULL se non richiesto.
 * @param[out] upper                Valore della soglia superiore in °C, NULL se non richiesto.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_get_celsius_threshold(max31865_t *device, float *lower, float *upper);

/**
 * @brief Imposta i limiti delle soglie di temperatura in gradi Celsius.
 * I parametri "lower" e "upper" non possono essere entrambi NULL
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] lower                 Valore della soglia inferiore in °C.
 * @param[in] upper                 Valore della soglia superiore in °C.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_set_celsius_threshold(max31865_t *device, const float lower, const float upper);

/**
 * @brief Restituisce lo stato del ciclo di rilevamento dei fault.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] state                Stato attuale del ciclo (0: Rilevamento terminato,
 * 1: Rilevamento automatico in esecuzione, 2: Ciclo 1 del rilevamento manuale in esecuzione,
 * 3: Ciclo 2 del rilevamento manuale in esecuzione)
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_read_fault_detection_cycle(max31865_t *device, uint8_t *state);

/**
 * @brief Avvia il ciclo di rilevamento dei fault. Nel caso in cui si utilizza la
 * modalità manuale, è importante poi terminarla con l'apposita funzione. Questo
 * non è necessario con la modalità automatica.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] mode                  Modalità di relevamento (0: automatica, 1: manuale).
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_start_fault_detection(max31865_t *device, const uint8_t mode);

/**
 * @brief Termina il ciclo di rilevamento dei fault, nel caso in cui
 * sia stata utilizzata la modalità manuale.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_stop_fault_detection(max31865_t *device);

/**
 * @brief Restituisce lo stato di fault.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[out] fault                Stato dei bit del fault.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_get_fault_status(max31865_t *device, uint8_t *fault);

/**
 * @brief Ripristina lo stato di fault eliminando tutti gli errori.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_clear_fault_status(max31865_t *device);

/**
 * @brief Avvia una singola conversione di temperatura.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_start_single_conversion(max31865_t *device);

/**
 * @brief Avvia le conversioni di temperatura in modalità continua.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_start_conversion(max31865_t *device);

/**
 * @brief Termina le conversioni di temperatura in modalità
 * continua e disabilita il bias per risparmiare energia.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_stop_conversion(max31865_t *device);

/**
 * @brief Restituisce lo stato di conversione nel caso in cui si utilizza la modalità singola e non continua.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] ready                 Valore dello stato (0: Dati non pronti, 1: Dati pronti)
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_is_data_ready(max31865_t *device, uint8_t *ready);

/**
 * @brief Questa funzione salva nella struttura del dispositivo i puntatori delle funzioni da utilizzare in caso di interrupt.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] callback              Contiene la funzione da eseguire quando arriva l'interrupt.
 * @param[in] context               Indica il contesto da passare alla funzione.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
max31865_err_t max31865_register_irq_callback(max31865_t *device, max31865_irq_callback_t callback, void *context);

#ifdef __cplusplus
}
#endif

#endif // MAX31865_H