// SPDX-License-Identifier: MIT


#ifndef MAX31865_PLATFORM_H
#define MAX31865_PLATFORM_H

#include "max31865_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inizializza l'interfaccia hardware del dispositivo MAX31865.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] bus_handle            Handle del bus SPI.
 * @param[in] spi_cs_pin            Pin CS del protocollo SPI.
 * @param[in] spi_clk_speed         Velocità di clock del dispositivo sul bus SPI.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore nella configurazione dell'ISR o del pin.
 */
max31865_err_t max31865_init_hal(
    max31865_t *device,
    spi_host_device_t bus_handle,
    const uint8_t spi_cs_pin,
    const uint32_t spi_clk_speed
);


/**
 * @brief Configura il pin di interrupt e registra la ISR.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] int_pin               Valore del pin da configurare.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 * @retval MAX31865_ERR_FAIL        Errore nella configurazione dell'ISR o del pin.
 */
max31865_err_t max31865_hal_setup_int(max31865_t *device, const uint8_t int_pin);

#ifdef __cplusplus
}
#endif

#endif // MAX31865_PLATFORM_H