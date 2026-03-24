// SPDX-License-Identifier: MIT

#include <math.h>
#include "max31865_core.h"

/**
 * @brief Esegue una funzione e se restituisce
 * errore, lo propaga interrompendo l'esecuzione.
 */
#define MAX31865_ERROR_CHECK(func)      \
    do {                                \
        max31865_err_t __err = (func);  \
        if (__err != MAX31865_ERR_OK)   \
            return __err;               \
    } while (0)


static const uint16_t MAX31865_ADC_RESOLUTION = 1 << 15;    // Il MAX31865 ha un ADC a 15 bit (2^15 = 32768)

/**
 * @brief Definizione dei registri.
 */
static const uint8_t REG_WRITE_OFFSET = 0x80;

static const uint8_t REG_CONFIG = 0x00;         // R/W
static const uint8_t REG_RTD_MSB = 0x01;        // R
static const uint8_t REG_RTD_LSB = 0x02;        // R
static const uint8_t REG_HIGH_FAULT_MSB = 0x03; // R/W
static const uint8_t REG_HIGH_FAULT_LSB = 0x04; // R/W
static const uint8_t REG_LOW_FAULT_MSB = 0x05;  // R/W
static const uint8_t REG_LOW_FAULT_LSB = 0x06;  // R/W
static const uint8_t REG_FAULT_STATUS = 0x07;   // R

/**
 * @brief Definizione delle maschere
 */
static const uint8_t MASK_CONFIG_BIAS = 0x80;
static const uint8_t MASK_CONFIG_CONVERSION = 0x40;
static const uint8_t MASK_CONFIG_ONE_SHOT = 0x20;
static const uint8_t MASK_CONFIG_WIRE = 0x10;
static const uint8_t MASK_CONFIG_FAULT_DETECTION = 0x0C;
static const uint8_t MASK_CONFIG_FAULT_STATUS = 0x02;
static const uint8_t MASK_CONFIG_FILTER = 0x01;

/**
 * @brief Costanti per il calcolo della temperatura
 */
static const float RTD_A = 3.9083e-3;
static const float RTD_B = -5.775e-7;
static const float RTD_C = -4.18301e-12;

/**
 * @brief Legge un byte (8 bit) da un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[out] data                 Dati da leggere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t read_register_8(max31865_t *device, uint8_t reg, uint8_t *data)
{
    // Verifica i parametri
    if (!device || !data)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore non è impostato
    // REG_WRITE_OFFSET-1 = 0x7F = 0b01111111, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg &= (REG_WRITE_OFFSET - 1);

    MAX31865_ERROR_CHECK(
        device->platform->spi_read(device, reg, 1, data)
    );

    return MAX31865_ERR_OK;
}

/**
 * @brief Scive un byte (8 bit) in un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] data                  Dati da scrivere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t write_register_8(max31865_t *device, uint8_t reg, const uint8_t data)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore sia impostato
    // REG_WRITE_OFFSET = 0x80 = 0b10000000, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg |= REG_WRITE_OFFSET;

    MAX31865_ERROR_CHECK(
        device->platform->spi_write(device, reg, 1, &data)
    );

    return MAX31865_ERR_OK;
}

/**
 * @brief Legge 2 byte (16 bit) da un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[out] data                 Dati da leggere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t read_register_16(max31865_t *device, uint8_t reg, uint16_t *data)
{
    // Verifica i parametri
    if (!device || !data)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore non è impostato
    // REG_WRITE_OFFSET-1 = 0x7F = 0b01111111, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg &= (REG_WRITE_OFFSET - 1);

    uint8_t buffer[2];
    MAX31865_ERROR_CHECK(
        device->platform->spi_read(device, reg, 2, buffer)
    );

    // Combina i byte per ricostruire il dato (big-endian: MSB first)
    *data = ((uint16_t)buffer[0] << 8) |
            ((uint16_t)buffer[1]);

    return MAX31865_ERR_OK;
}

/**
 * @brief Scive 2 byte (16 bit) in un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] data                  Dati da scrivere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t write_register_16(max31865_t *device, uint8_t reg, const uint16_t data)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore sia impostato
    // REG_WRITE_OFFSET = 0x80 = 0b10000000, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg |= REG_WRITE_OFFSET;

    // Definisce il buffer (big-endian: MSB first)
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(data >> 8);   // MSB
    buffer[1] = (uint8_t)(data & 0xFF); // LSB

    MAX31865_ERROR_CHECK(
        device->platform->spi_write(device, reg, 2, buffer)
    );

    return MAX31865_ERR_OK;
}

/**
 * @brief Legge 4 byte (32 bit) da un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in]  device               Dispositivo MAX31865.
 * @param[in]  reg                  Indirizzo del registro.
 * @param[out] data                 Dati da leggere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t read_register_32(max31865_t *device, uint8_t reg, uint32_t *data)
{
    // Verifica i parametri
    if (!device || !data)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore non è impostato
    // REG_WRITE_OFFSET-1 = 0x7F = 0b01111111, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg &= (REG_WRITE_OFFSET - 1);

    uint8_t buffer[4];
    MAX31865_ERROR_CHECK(
        device->platform->spi_read(device, reg, 4, buffer)
    );

    // Combina i byte per ricostruire il dato (big-endian: MSB first)
    *data = ((uint32_t)buffer[0] << 24) |
            ((uint32_t)buffer[1] << 16) |
            ((uint32_t)buffer[2] << 8)  |
            ((uint32_t)buffer[3]);

    return MAX31865_ERR_OK;
}

/**
 * @brief Scive 4 byte (32 bit) in un registro del dispositivo, tramite il protocollo SPI.
 *
 * @param[in] device                Dispositivo MAX31865.
 * @param[in] reg                   Indirizzo del registro.
 * @param[in] data                  Dati da scrivere.
 * @retval MAX31865_ERR_OK          Successo.
 * @retval MAX31865_ERR_INVALID_ARG Parametri non validi.
 */
static max31865_err_t write_register_32(max31865_t *device, uint8_t reg, const uint32_t data)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // La maschera assicura che il bit superiore sia impostato
    // REG_WRITE_OFFSET = 0x80 = 0b10000000, questo perché il
    // dispositivo MAX31865 utilizza il bit 7 del registro così:
    //  - MSB = 1 -> scrittura
    //  - MSB = 0 -> lettura
    reg |= REG_WRITE_OFFSET;

    // Definisce il buffer (big-endian: MSB first)
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(data >> 24);  // MSB
    buffer[1] = (uint8_t)(data >> 16);
    buffer[2] = (uint8_t)(data >> 8);
    buffer[3] = (uint8_t)(data & 0xFF); // LSB

    MAX31865_ERROR_CHECK(
        device->platform->spi_write(device, reg, 4, buffer)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_init_core(
    max31865_t *device,
    const float reference_resistor,
    const float rtd_nominal_resistance,
    const uint8_t wire_number,
    const uint8_t filter_mode
)
{
    // Verifica i parametri
    if (!device || wire_number > 1 || filter_mode > 1)
        return MAX31865_ERR_INVALID_ARG;

    device->reference_resistor = reference_resistor;
    device->rtd_nominal_resistance = rtd_nominal_resistance;

    // Elimina lo stato di guasto del sensore
    MAX31865_ERROR_CHECK(
        max31865_clear_fault_status(device)
    );

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Resetta tutti i bit da configurare
    reg_val &= ~(
        MASK_CONFIG_WIRE |
        MASK_CONFIG_FILTER
    );

    // Configura il numero di cavi della sonda
    if (wire_number)
        reg_val |= MASK_CONFIG_WIRE;

    // Configura la modalità del filtro
    if (filter_mode)
        reg_val |= MASK_CONFIG_FILTER;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_get_config(max31865_t *device, uint8_t *value)
{
    // Verifica i parametri
    if (!device || !value)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    *value = reg_val;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_set_config(max31865_t *device, const uint8_t bitmask, const uint8_t value)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Imposta il valore dei pin specificati:
    // (config & ~bitmask) azzera i valori dei pin specificati
    // (value & bitmask) mantiene i soli valori dei pin specificati
    // L'OR modifica lo stato precedente con la nuova configurazione
    reg_val = (reg_val & ~bitmask) | (value & bitmask);

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_rtd_to_celsius(max31865_t *device, const uint16_t rtd, float *celsius)
{
    // Verifica i parametri
    if (!device || !celsius || rtd >= MAX31865_ADC_RESOLUTION)
        return MAX31865_ERR_INVALID_ARG;

    // Calcola il valore r_rtd
    float r_rtd = (rtd * device->reference_resistor) / MAX31865_ADC_RESOLUTION;

    // La seguente formula è valida solo per valori sopra 0°C (compreso)
    float z1 = -RTD_A;                                          // z1 = -3.9083e-3
    float z2 = (RTD_A * RTD_A) - (4.0f * RTD_B);                // z2 = 17.58480889e-6
    float z3 = (4.0f * RTD_B) / device->rtd_nominal_resistance; // z3 = -23.1e-9
    float z4 = 2.0f * RTD_B;                                    // z4 = -1.155e-6
    float square_root = z2 + (z3 * r_rtd);

    if (square_root < 0.0f)
        square_root = 0.0f;

    *celsius = (z1 + sqrtf(square_root)) / z4;

    if (*celsius >= 0.0f)
        return MAX31865_ERR_OK;

    // La seguente formula è valida solo per valori sotto 0°C (escluso)
    // I polinomi sono stati calcolati con il metodo Horner per risparmiare moltiplicazioni

    r_rtd /= device->rtd_nominal_resistance;
    r_rtd *= 100.0f;
    float r = r_rtd;

    // Approssimazione polinomiale di quinto grado (+0.0001°C/-0.00005°C)
    *celsius = -242.02f + r * (2.2228f + r * (2.5859e-3f + r * (-4.8260e-6f + r * (-2.8183e-8f + r * 1.5243e-10f))));

    // Approssimazione polinomiale di quarto grado (+0.0022°C/-0.001°C)
    // *celsius = -241.96f + r * (2.2163f + r * (2.8541e-3f + r * (-9.9121e-6f + r * -1.7052e-8f)));

    // Approssimazione polinomiale di terzo grado (+0.0053°C/-0.0085°C)
    // *celsius = -242.09f + r * (2.2276f + r * (2.5178e-3f + r * -5.8620e-6f));

    // Approssimazione polinomiale di secondo grado (+0.075°C/-0.17°C)
    // *celsius = -242.97f + r * (2.2838f + r * 1.4727e-3f);

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_celsius_to_rtd(max31865_t *device, float celsius, uint16_t *rtd)
{
    // Verifica i parametri
    if (!device || !rtd)
        return MAX31865_ERR_INVALID_ARG;

    // La seguente formula è valida solo per valori sopra 0°C (compreso)
    float r_rtd = device->rtd_nominal_resistance * (1.0f + (RTD_A * celsius) + (RTD_B * celsius * celsius));

    // La seguente formula viene aggiunta solo per valori sotto 0°C (escluso)
    if (celsius < 0.0f)
        r_rtd += RTD_C * (celsius - 100.0f) * powf(celsius, 3.0f);

    // Conversione della resistenza RTD: Ohm -> ADC_CODE
    *rtd = lroundf((r_rtd * MAX31865_ADC_RESOLUTION) / device->reference_resistor);

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_read_rtd(max31865_t *device, uint16_t *rtd, uint8_t *fault)
{
    // Verifica i parametri
    if (!device || !rtd)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro RTD (MSB + LSB)
    uint16_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_16(device, REG_RTD_MSB, &reg_val)
    );

    // Struttura del registro RTD:
    //  - Bit [15-1], è il valore RTD
    //  - Bit 0, è lo stato di guasto

    // Converte il valore RTD da 16 bit a 15 bit
    *rtd = reg_val >> 1;

    if (fault)
        // Applica la maschera per ottenere il solo stato di fault
        *fault = reg_val & 0x01;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_get_rtd_threshold(max31865_t *device, uint16_t *lower, uint16_t *upper)
{
    // Verifica i parametri
    if (!device || (!lower && !upper))
        return MAX31865_ERR_INVALID_ARG;

    // Legge i registri Fault Threshold (high e low)
    uint32_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_32(device, REG_HIGH_FAULT_MSB, &reg_val)
    );

    if (lower)
        // Estrae i 16 bit LSB e converte da 16 a 15 bit
        *lower = ((uint16_t)(reg_val & 0xFFFF) >> 1);

    if (upper)
        // Estrae i 16 bit MSB e converte da 16 a 15 bit
        *upper = ((uint16_t)(reg_val >> 16) >> 1);

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_set_rtd_threshold(max31865_t *device, uint16_t lower, uint16_t upper)
{
    // Verifica i parametri
    if (!device || upper < lower)
        return MAX31865_ERR_INVALID_ARG;

    // Maschera i valori RTD a 15 bit e li converte a 16 bit
    lower = (lower & 0x7FFF) << 1;
    upper = (upper & 0x7FFF) << 1;

    uint32_t reg_val = ((uint32_t)upper << 16) | ((uint32_t)lower & 0xFFFF);

    // Scrive i registri di Fault Threshold (high e low)
    MAX31865_ERROR_CHECK(
        write_register_32(device, REG_HIGH_FAULT_MSB, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_read_celsius(max31865_t *device, float *celsius, uint8_t *fault)
{
    // Verifica i parametri
    if (!device || !celsius)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro RTD (MSB + LSB)
    uint16_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_16(device, REG_RTD_MSB, &reg_val)
    );

    // Struttura del registro RTD:
    //  - Bit [15-1], è il valore RTD
    //  - Bit 0, è lo stato di guasto

    // Converte il valore RTD in gradi Celsius
    float temperature = 0.0f;
    MAX31865_ERROR_CHECK(
        max31865_rtd_to_celsius(device, reg_val >> 1, &temperature)
    );

    *celsius = temperature;

    if (fault)
        // Applica la maschera per ottenere il solo stato di fault
        *fault = reg_val & 0x01;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_get_celsius_threshold(max31865_t *device, float *lower, float *upper)
{
    // Verifica i parametri
    if (!device || (!lower && !upper))
        return MAX31865_ERR_INVALID_ARG;

    // Legge i registri Fault Threshold (high e low)
    uint32_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_32(device, REG_HIGH_FAULT_MSB, &reg_val)
    );

    if (lower)
    {
        // Estrae i 16 bit LSB e converte da 16 a 15 bit
        uint16_t low_rtd = ((uint16_t)(reg_val & 0xFFFF) >> 1);

        // Converte il valore RTD in gradi Celsius
        float celsius = 0.0f;
        MAX31865_ERROR_CHECK(
            max31865_rtd_to_celsius(device, low_rtd, &celsius)
        );

        *lower = celsius;
    }

    if (upper)
    {
        // Estrae i 16 bit MSB e converte da 16 a 15 bit
        uint16_t high_rtd = ((uint16_t)(reg_val >> 16) >> 1);

        // Converte il valore RTD in gradi Celsius
        float celsius = 0.0f;
        MAX31865_ERROR_CHECK(
            max31865_rtd_to_celsius(device, high_rtd, &celsius)
        );

        *upper = celsius;
    }

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_set_celsius_threshold(max31865_t *device, const float lower, const float upper)
{
    // Verifica i parametri
    if (!device || upper < lower)
        return MAX31865_ERR_INVALID_ARG;

    uint16_t lower_rtd = 0;
    uint16_t upper_rtd = 0;

    // Converte i gradi Celsius in un valore RTD
    MAX31865_ERROR_CHECK(
        max31865_celsius_to_rtd(device, lower, &lower_rtd)
    );

    MAX31865_ERROR_CHECK(
        max31865_celsius_to_rtd(device, upper, &upper_rtd)
    );

    // Maschera i valori RTD a 15 bit e li converte a 16 bit
    lower_rtd = (lower_rtd & 0x7FFF) << 1;
    upper_rtd = (upper_rtd & 0x7FFF) << 1;

    uint32_t reg_val = ((uint32_t)upper_rtd << 16) | ((uint32_t)lower_rtd & 0xFFFF);

    // Scrive i registri di Fault Threshold (high e low)
    MAX31865_ERROR_CHECK(
        write_register_32(device, REG_HIGH_FAULT_MSB, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_read_fault_detection_cycle(max31865_t *device, uint8_t *state)
{
    // Verifica i parametri
    if (!device || !state)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Maschera e ottiene il valore dei bit D[3:2]
    reg_val = (reg_val & MASK_CONFIG_FAULT_DETECTION) >> 2;

    switch(reg_val)
    {
        // Rilevamento terminato
        case 0x00:
            *state = 0;
            break;

        // Rilevamento automatico in esecuzione
        case 0x01:
            *state = 1;
            break;

        // Ciclo 1 rilevamento manuale in esecuzione
        case 0x02:
            *state = 2;
            break;

        // Ciclo 2 rilevamento manuale in esecuzione
        case 0x03:
            *state = 3;
            break;

        default:
            break;
    }

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_start_fault_detection(max31865_t *device, const uint8_t mode)
{
    // Verifica i parametri
    if (!device || mode > 1)
        return MAX31865_ERR_INVALID_ARG;

    // Determina il tipo di rilevamento da utilizzare
    switch(mode)
    {
        case 0:
            // Per abilitare il rilevamento dei guasti con il delay automatico (100 us)
            // è necessario scrivere 0b100X010X nel registro di configurazione.
            // La X indica che quei bit possono assumere qualsiasi valore.
            MAX31865_ERROR_CHECK(
                max31865_set_config(device, 0b11101110, 0b10000100)
            );
            break;

        case 1:
            // Abilita il bias
            MAX31865_ERROR_CHECK(
                max31865_set_config(device, 0b10000000, 0b10000000)
            );

            // Attende 10 ms
            device->platform->delay_ms(10);

            // Per avviare il rilevamento dei guasti con il delay manuale
            // è necessario scrivere 0b100X100X nel registro di configurazione.
            // La X indica che quei bit possono assumere qualsiasi valore.
            MAX31865_ERROR_CHECK(
                max31865_set_config(device, 0b11101110, 0b10001000)
            );
            break;

        default:
            break;
    }

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_stop_fault_detection(max31865_t *device)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Maschera e ottiene il valore dei bit D[3:2]
    reg_val = (reg_val & MASK_CONFIG_FAULT_DETECTION) >> 2;

    // Verifica se è stato avviato il ciclo di rilevamento dei guasti manuale.
    if (reg_val == 0x02)
    {
        // Per terminare il rilevamento dei guasti con il delay manuale
        // è necessario scrivere 0b100X110X nel registro di configurazione.
        // La X indica che quei bit possono assumere qualsiasi valore.
        MAX31865_ERROR_CHECK(
            max31865_set_config(device, 0b11101110, 0b10001100)
        );
    }

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_get_fault_status(max31865_t *device, uint8_t *fault)
{
    // Verifica i parametri
    if (!device || !fault)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Fault Status
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_FAULT_STATUS, &reg_val)
    );

    *fault = reg_val;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_clear_fault_status(max31865_t *device)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Resetta tutti i bit specificati dalle maschere
    reg_val &= ~(
        MASK_CONFIG_ONE_SHOT |
        MASK_CONFIG_FAULT_DETECTION
    );

    // Elimina lo stato di fault
    reg_val |= MASK_CONFIG_FAULT_STATUS;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_start_single_conversion(max31865_t *device)
{
    // Verifica i parametri
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Abilita il bias
    reg_val |= MASK_CONFIG_BIAS;

    // Imposta la conversione auto su off
    reg_val &= ~MASK_CONFIG_CONVERSION;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    // Attende 10 ms per stabilizzare la lettura
    device->platform->delay_ms(10);

    // Legge il registro RTD per essere sicuri che il
    // pin di interrupt abbia il livello logico high
    uint16_t rtd = 0;
    MAX31865_ERROR_CHECK(
        read_register_16(device, REG_RTD_MSB, &rtd)
    );

    // Avvia la lettura della temperatura
    reg_val |= MASK_CONFIG_ONE_SHOT;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_start_conversion(max31865_t *device)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Abilita il bias
    MAX31865_ERROR_CHECK(
        max31865_set_config(device, 0b10000000, 0b10000000)
    );

    // Attende 10 ms dopo l'attivazione del bias
    device->platform->delay_ms(10);

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Imposta la conversione auto su on
    reg_val |= MASK_CONFIG_CONVERSION;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    // Legge il registro RTD per essere sicuri che il
    // pin di interrupt abbia il livello logico high
    uint16_t rtd = 0;
    MAX31865_ERROR_CHECK(
        read_register_16(device, REG_RTD_MSB, &rtd)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_stop_conversion(max31865_t *device)
{
    // Verifica il parametro
    if (!device)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Disabilita il bias
    reg_val &= ~MASK_CONFIG_BIAS;

    // Imposta la conversione auto su off
    reg_val &= ~MASK_CONFIG_CONVERSION;

    // Scrive il registro Configuration
    MAX31865_ERROR_CHECK(
        write_register_8(device, REG_CONFIG, reg_val)
    );

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_is_data_ready(max31865_t *device, uint8_t *ready)
{
    // Verifica i parametri
    if (!device || !ready)
        return MAX31865_ERR_INVALID_ARG;

    // Legge il registro Configuration
    uint8_t reg_val = 0;
    MAX31865_ERROR_CHECK(
        read_register_8(device, REG_CONFIG, &reg_val)
    );

    // Maschera e ottiene il valore del bit 5
    reg_val = (reg_val & MASK_CONFIG_ONE_SHOT) >> 5;

    // Inverte il valore
    *ready = reg_val ? 0 : 1;

    return MAX31865_ERR_OK;
}


max31865_err_t max31865_register_irq_callback(max31865_t *device, max31865_irq_callback_t callback, void *context)
{
    // Verifica i parametri
    if (!device || !callback || !context)
        return MAX31865_ERR_INVALID_ARG;

    device->irq_callback = callback;
    device->irq_context = context;

    return MAX31865_ERR_OK;
}
