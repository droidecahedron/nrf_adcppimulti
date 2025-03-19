# nrf_adcppimulti (APM)

Zephyr-based application for nRF SoC (specifically nRF54L15) using nrfx & utilizing DPPI (distributed programmable peripheral interconnect) to connect the ADC peripheral to the TIMER peripheral and sample multiple ADC channels with minimal CPU involvement.

# Supported Hardware
| Compatible devices|
|---|
| **nRF54L15DK** |
| nRF52832DK |
| nRF52840DK|
| nRF5340DK|
> Frankly, any Nordic DK should do, but this sample is especially for the nRF54L15.

# Pinout
![image](https://github.com/user-attachments/assets/c8ca8f09-b566-4f76-bf32-9da619666614)
|AINx|Port.Pin|
|---|---|
|`AIN0`|`P1.04`|
|`AIN1`|`P1.05`|
|`AIN2`|`P1.06`|
|`AIN3`|`P1.07`|
|`AIN4`|`P1.11`|
|`AIN5`|`P1.12`|
|`AIN6`|`P1.13`|
|`AIN7`|`P1.14`|
> There are limitations around ports and pins.
> The device power domains have their own GPIO ports with different capabilities. See [**HERE**](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/gpio.html#ariaid-title4)

# Example output
![image](https://github.com/user-attachments/assets/c312edab-90a1-4053-a185-c1b374bcffb0)

# Notes
> Important: when sampling multiple channels, the data in the buffer will be interleaved - first sample from channel 0, then first sample from channel 1, and so on.
> There's no good way to implement different channels that you sample at different frequencies. You'll need to either sample all channels at the same frequency or reconfigure the SAADC module when you want to change the sampling configuration.

# Relevant manpages
[nRF54L15 DPPI](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/dppi.html)
[nRF54L15 Pins](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/chapters/pin.html)
[nRF54L15 SAADC](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/saadc.html#ariaid-title49)

## Special thanks
@hlord2000 for https://github.com/hlord2000/hlord2000.github.io and helping me avoid turning my head 90 degrees to read pin names
