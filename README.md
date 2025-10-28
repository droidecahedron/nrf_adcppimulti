# nrf_adcppimulti (APM)

Zephyr-based application for nRF SoC (specifically nRF54L15) using nrfx & utilizing DPPI (distributed programmable peripheral interconnect) to connect the ADC peripheral to the TIMER peripheral and sample multiple ADC channels with minimal CPU involvement.

# TEST SAR
> [!IMPORTANT]
> It may seem confusing that when you set resolution to 12 or 14 without oversampling (the RESOLUTION register only supports 8,10,12,14), you always get even results.
>
> The ADC is actually a 11-bit ADC. That means SAADC has 11 arithmetic bits in differential mode. You may mix up with this -- when it is single-ended mode, only half of the input range has been in use, thus, only 10 bits are valid (1/2 of the 2^11).
>
> “This behavior does not persist when using 8bit and 10bit for RESOLUTION. Only 12 and 14.” For example, when resolution is set as 12, differential mode is selected.
>
> Without oversampling, a fake/void LSB is added to the real 11 bits for REGRESULT output.
>
> From the SAADC PS: 10-bit resolution in single-ended mode, 11-bit resolution in differential mode, and 12/14-bit resolution with oversampling.

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
![image](https://github.com/user-attachments/assets/85f90a0a-bcb4-42b6-8d97-8c421d901f70)


## Testing with a waveform generator
![image](https://github.com/user-attachments/assets/c580dcb5-7ef5-4b8f-8b4d-0a08ffa65f97)


# Notes
> Important: when sampling multiple channels, the data in the buffer will be interleaved - first sample from channel 0, then first sample from channel 1, and so on.
> There's no good way to implement different channels that you sample at different frequencies. You'll need to either sample all channels at the same frequency or reconfigure the SAADC module when you want to change the sampling configuration.
> **Calibration:** The ADC has a temperature dependent offset. If the ADC is to operate over a large temperature range, we recommend running CALIBRATEOFFSET at regular intervals. The CALIBRATEDONE event will be fired when the calibration has been completed. Note that the DONE and RESULTDONE events will also be generated.

# Relevant manpages
[nRF54L15 DPPI](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/dppi.html)
[nRF54L15 Pins](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/chapters/pin.html)
[nRF54L15 SAADC](https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/saadc.html#ariaid-title49)

## Special thanks
@hlord2000 for https://github.com/hlord2000/hlord2000.github.io and helping me avoid turning my head 90 degrees to read pin names
