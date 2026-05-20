## :package: MAX31865 HAL Driver Library
HAL Driver Library for the MAX31865 RTD Temperature Sensor.

This driver has been developed to be hardware-independent.
This way, the functions present in the `core` can be used as-is on any other microcontroller.
The only functions that need to be adapted are those contained in the `platform` folder, as these are the ones that interface with and depend on the hardware.
Currently, only the `ESP32` implementation is available.

## :bulb: Examples
To get started with the driver, check out the `examples` folder for practical usage references.
This folder contains the following files for each available microcontroller:
- `max31865_basic.c`
- `max31865_polling.c`
- `max31865_interrupt.c`

## :books: References

- [MAX31865 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max31865.pdf)
- [MAX31865 Conversion formulas](https://www.analog.com/media/en/technical-documentation/application-notes/AN709_0.pdf)
- [MAX31865 Wiring](https://learn.adafruit.com/adafruit-max31865-rtd-pt100-amplifier/rtd-wiring-config)

## :heavy_exclamation_mark: Disclaimer
This software is provided **"AS IS"**, without warranty of any kind.
The author(s) accept no responsibility for any damage to hardware, data loss, device malfunction, or any other direct or indirect consequences arising from the use or misuse of this code.

**Use at your own risk.** It is your sole responsibility to ensure that this software is appropriate and safe for your specific hardware and use case.

This project is licensed under the [MIT License](LICENSE).