# Arduino RFID Intruder Alarm

**Please note: This is a *work in progress*. Features may not work as intended and more features are planned to be added.**

## Stuff required

### Equipment
*Not including breadboard/wire/PCB/solder/casing/however you choose to make this.*
- Arduino (I used Uno, but for real-world usage a Mega etc. would be better)
- 16x2 LCD with I2C board (once again, bigger would be better)
- RC522 RFID module
- Relay (relay *board*, in my case)
- Passive buzzer
- A power source
- Any sensors you want to add - unless you modify the code, they must be NC (Normally Closed).

### Non-stock libraries
- MFRC522
- LiquidCrystal I2C



## Pins

### RFID Reader (RC522)

|Arduino Pin|RC522 Pin|
|-----------|---------|
|3.3V|3.3V|
|GND|GND|
|Not connected|IRQ|
|9|RST|
|10|SDA|
|11|MOSI|
|12|MISO|
|13|SCK|

### I^2C Display Module

The I2C module is wired directly to the screen, and then connected to the Arduino as below:

|Arduino Pin|Display pin|
|5V|VCC|
|GND|GND|
|A4|SDA|
|A5|SCL|

### Relay

The relay for the siren output is wired to digital pin **8**. In my case the relay was on a board which also had to be connected to 5V and GND, however if you are using a "plain" relay it will only need pin 8 and GND.

The NO output of the relay will need to be wired to control a siren/flashing light/auto dialler/etc. I recommend using only a 12V siren for safety, but theoretically 240V should also be safe if using a relay rated for such voltages.

### Buzzer

This must be a passive buzzer - an active buzzer may make a sound, but it cannot produce different pitches as this code is designed to do. It is wired across **digital pin 7** and GND.

### Sensors

Currently there are 4 zones. These are digital pins **2, 3, 4** and **5**. All are "INPUT\_PULLUP" in the code - in other words, they are Normally Closed. This matches most commercially available security sensors, as well as most DIY options such as microswitches placed under objects, so should be suitable for most applications.

## Configurable Constants etc

Anything in this code can obviously be changed, but as for the most important options:

- **POLLING_RATE**: This is the polling rate for alarm and RFID inputs in Hz. It is 10 by default but can be modified if you feel the need.
- **-PIN constants** - Any pins can be changed, so long as the newly chosen pins are still suitable.
- **ZONE_PINS** - You can add as many pins as you like to this - just make sure you update ZONE\_QTY.
- **EE_TIME** - Entry/exit delay time in *milliseconds*
- **INTRUDER_LOCKOUT_TIME** - Time (milliseconds) that the alarm will refuse arm/disarm attempts after an intruder lockout trigger.
- **INTRUDER_LOCKOUT_THRESHOLD** - Maximum unsuccessful login attempts before intruder lockout kicks in.


