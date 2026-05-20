/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2026 Reptaide.
 *
 * Released under the terms of the MIT License.
 * See LICENSE in the root of this repository for the full license text.
 */

#ifndef MAX31865_CORE_H
#define MAX31865_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct max31865_t max31865_t;

    typedef enum
    {
        MAX31865_ERR_OK = 0,
        MAX31865_ERR_FAIL,
        MAX31865_ERR_INVALID_ARG,
    } max31865_err_t;

    typedef max31865_err_t (*max31865_read_register_t)(
        void *handle, const uint8_t reg, const size_t length, uint8_t *data);
    typedef max31865_err_t (*max31865_write_register_t)(
        void *handle, const uint8_t reg, const size_t length, const uint8_t *data);
    typedef void (*max31865_time_delay_t)(const uint32_t ms);
    typedef void (*max31865_isr_handler_t)(max31865_t *device, void *context);

    typedef struct
    {
        max31865_read_register_t spi_read;
        max31865_write_register_t spi_write;
        max31865_time_delay_t delay_ms;
    } max31865_platform_t;

    struct max31865_t
    {
        void *spi_bus_handle;                // Handler del bus SPI
        void *spi_device_handle;             // Handler dispositivo SPI
        uint8_t spi_cs_pin;                  // Pin del chip-select
        uint32_t spi_clk_speed;              // Velocità clock SPI
        float reference_resistor;            // Resistenza di riferimento in Ohm
        float rtd_nominal_resistance;        // Resistenza nominale RTD a 0 °C in Ohm
        int8_t int_pin;                      // Pin di interrupt
        void *context;                       // Puntatore generico a servizio dell'applicazione
        max31865_isr_handler_t isr_handler;  // Funzione chiamata all'arrivo di un interrupt
        const max31865_platform_t *platform; // Puntatore alla struttura platform
    };

    /**
     * @brief Inizializza il core del dispositivo MAX31865.
     *
     * @param[in] device                    Dispositivo MAX31865.
     * @param[in] reference_resistor        Valore della resistenza di riferimento.
     * @param[in] rtd_nominal_resistance    Valore della resistenza RTD.
     * @param[in] wire_number               Numero cavi della sonda (0: 2 o 4 cavi, 1: 3 cavi).
     * @param[in] filter_mode               Frequenza di campionamento (0: 60 Hz, 1: 50 Hz).
     * @retval MAX31865_ERR_OK              Successo.
     * @retval MAX31865_ERR_INVALID_ARG     Parametri non validi.
     */
    max31865_err_t max31865_init_core(max31865_t *device,
        const float reference_resistor,
        const float rtd_nominal_resistance,
        const uint8_t wire_number,
        const uint8_t filter_mode);

    /**
     * @brief Ottiene lo stato dal registro di configurazione.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] value                Stato dei bit della configurazione.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_get_config(max31865_t *device, uint8_t *value);

    /**
     * @brief Imposta lo stato del registro di configurazione.
     *
     * @param[in] device                Dispositivo MAX31865.
     * @param[in] bitmask               Bitmask dei bit da modificare.
     * @param[in] value                 Bitmask dello stato logico dei pin.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_set_config(
        max31865_t *device, const uint8_t bitmask, const uint8_t value);

    /**
     * @brief Converte una resistenza RTD in gradi Celsius.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[in]  rtd                  Valore RTD.
     * @param[out] celsius              Valore della temperatura in °C
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_rtd_to_celsius(max31865_t *device, const uint16_t rtd, float *celsius);

    /**
     * @brief Converte una temperatura in gradi Celsius in una resistenza RTD.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[in]  celsius              Valore della temperatura in °C.
     * @param[out] rtd                  Valore della resistenza RTD.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_celsius_to_rtd(max31865_t *device, float celsius, uint16_t *rtd);

    /**
     * @brief Ottiene il valore della resistenza RTD e lo stato di fault.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] rtd                  Valore della resistenza RTD.
     * @param[out] fault                Stato di fault, NULL se non richiesto.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_read_rtd(max31865_t *device, uint16_t *rtd, uint8_t *fault);

    /**
     * @brief Ottiene i limiti delle soglie di temperatura in formato RTD.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] lower                Soglia inferiore in RTD, NULL se non richiesto.
     * @param[out] upper                Soglia superiore in RTD, NULL se non richiesto.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_get_rtd_threshold(max31865_t *device, uint16_t *lower, uint16_t *upper);

    /**
     * @brief Imposta i limiti delle soglie di temperatura in formato RTD.
     *
     * @param[in] device                Dispositivo MAX31865.
     * @param[in] lower                 Soglia inferiore in RTD.
     * @param[in] upper                 Soglia superiore in RTD.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_set_rtd_threshold(max31865_t *device, uint16_t lower, uint16_t upper);

    /**
     * @brief Ottiene il valore della temperatura in gradi Celsius e lo stato di fault.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] celsius              Valore della temperatura in °C.
     * @param[out] fault                Stato di fault, NULL se non richiesto.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_read_celsius(max31865_t *device, float *celsius, uint8_t *fault);

    /**
     * @brief Ottinene i limiti delle soglie di temperatura in gradi Celsius.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] lower                Soglia inferiore in °C, NULL se non richiesto.
     * @param[out] upper                Soglia superiore in °C, NULL se non richiesto.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_get_celsius_threshold(max31865_t *device, float *lower, float *upper);

    /**
     * @brief Imposta i limiti delle soglie di temperatura in gradi Celsius.
     *
     * @param[in] device                Dispositivo MAX31865.
     * @param[in] lower                 Soglia inferiore in °C.
     * @param[in] upper                 Soglia superiore in °C.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_set_celsius_threshold(
        max31865_t *device, const float lower, const float upper);

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
     * @param[out] fault                Bitmask dei bit di fault.
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
     * @brief Avvia in modalità continua conversioni di temperatura.
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
     * @brief Restituisce lo stato di conversione nel caso di singola conversione.
     *
     * @param[in]  device               Dispositivo MAX31865.
     * @param[out] value                Flag dati pronti (1: pronti, 0: altrimenti).
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_is_data_ready(max31865_t *device, uint8_t *value);

    /**
     * @brief Configura la funzione chiamata al verificarsi di un interrupt hardware.
     *
     * @param[in] device                Dispositivo MAX31865.
     * @param[in] handler               Funzione invocata dalla ISR.
     * @param[in] context               Contesto passato alla callback ISR. NULL se inutilizzato.
     * @retval MAX31865_ERR_OK          Successo.
     * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
     */
    max31865_err_t max31865_set_isr_handler(
        max31865_t *device, max31865_isr_handler_t handler, void *context);

#ifdef __cplusplus
}
#endif

#endif // MAX31865_CORE_H