# Project for replacing the CPU of a CASIO Label printer KL-780 with an ESP32

## Description

Mainly there are three things. First the motor driver with a PWM and a readback with a light barrier. Second the thermoelement with the shift register and the write enable. Third the catirge detection with 3 pins.

### Motor driver

A simple PWM regulated by the light barrier to have a controlled printing speed. The measurement shows, that the light barrier is regulated to 220Hz. The PWM has a main frequenzy of 15kHz and has a hold time around 25%. This will be differen for different tapes and devices, because it is regulated. To move after printing it has a hold time of around 75% with the light barrier at 440Hz.

### thermoelement

4 Pins are used for generating the image

1. clock (2MHz)
1. Data (changing on falling edge, latched on rising edge)
1. _Latch (short low pulse to latch the data)
1. _Strobe (to enable the heater 3.15ms at my printer)

The data could be clocked in when ever you want. Set _Latch to low will transfer the register content to the heater. There are 12 Byte with 8 bit = 96 pixel to be set. It will be done twice with the even bits and the odd bits directly after each other with its own strobe. The last bits for the bottom and the first Bits for the top. That mean 96 pixel for 18mm, 84 pixel for 12mm and xx pixel for 6mm. Every fourth light barrier pulse two 3.2ms heats are applied.

### catridge detection

There are 3 Pins. Two for tape type and one for tape insertion. Low means pin is pressed.

| Pin | Level | Result  |
|-----|-------|---------|
| TD  | high  | no tape |
| TD  | low   | no tape |
| T1  | high  | 6 mm    |
| T1  | low   | 12 mm   |
| T2  | high  | 12 mm   |
| T2  | low*  | 18 mm   |

 \* needs also T1 low

### resolution

After 4 rising edges the original is printing (heating) 150 pulses (4*150) result in 18.5mm print length. That result in a horizontal resolution for every edge 18.5 mm / 600 pulses = 0.031 mm/pulse = 3/100 mm / pulse. I think the heating capacity holds the temp that long, that this resolution could not be reached. This result in 0.124mm/4pulse. So I start with this 4 pulses like the original. 220 Hz result in 6.6mm/s.

Vertical we have 96 bits on 12mm = 0.125mm/bit.

So hope I have a calculation error and the resolution is horizontal and vertical identical.

### memory and print length

The programm uses 37828 byte RAM from 532480, with only one buffer for the 12 Bytes. Let`s assume, that 400000 are free at the end, when all is programmed. 400.000 / 12 = 33333 lines are possible to store. 33333 * 0.125mm/pulse = 4166 mm. 4m? Who will print that? That mean the memory is quite enough for that. Go the way round. 25cm is a long text for such a lable. 250mm/0.125mm/pulse = 2000 pulses. 12 Bytes * 2000 = 24000 Bytes. So I will reserve a Buffer of 32k.

## Disclaimer

Casio is a registered trademark of Casio Computer Co., Ltd.  
This project is not affiliated with, endorsed by, or in any way officially connected with Casio Computer Co., Ltd.  
The use of the name "Casio" is solely for descriptive purposes to identify compatibility or usage with Casio products.