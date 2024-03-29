# Arduino RFID Intruder Alarm

**Please note: This is a *work in progress*. Features may not work as intended and more features are planned to be added.**
**This is *not* intended for use as a serious security device. In addition, the RFID protocol used by this is very outdated
and should not be relied upon for any real-world security application.**

## Stuff required

### Components
*Not including breadboard/wire/PCB/solder/casing/however you choose to make this.*
- Arduino (Uno is sufficient, but there's nothing stopping you from using a better model and increasing the number of zones.)
- 16x2 LCD with I2C board
- RC522 RFID module
- Relay
- Passive buzzer
- A power source
- 10K resistor
- Momentary push button
- Any sensors you want to add - unless you modify the code, they must be NC (Normally Closed).

### Non-stock libraries
- MFRC522
- LiquidCrystal I2C

### EEPROM pre-writing
Right now, this project has no provision to add the admin card (user ID 0). I am working on this, but in the mean time, you will have to 
write it yourself prior to flashing this program. Simply write the UID of the card to the first 4 or 7 (depending on the card type) bytes
of the EEPROM. 

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
|-----------|-----------|
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

### Admin button

A button is used to add and remove cards. This is connected to **digital pin 7** and is wired using a 10K pull-down resistor - the same as in the Arduino button wiring guide.

## Configurable Constants etc

Anything in this code can obviously be changed, but as for the most important options:

- **POLLING_RATE**: This is the polling rate for alarm and RFID inputs in Hz. It is 10 by default but can be modified if you feel the need.
- **-PIN constants** - Any pins can be changed, so long as the newly chosen pins are still suitable.
- **ZONE_PINS** - You can add as many pins as you like to this - just make sure you update ZONE\_QTY.
- **DELAY_ENABLE** - Whether or not entry delay will be applied to each zone. Set all entry/exit zones to true.
- **EE_TIME** - Entry/exit delay time in *milliseconds*
- **INTRUDER_LOCKOUT_TIME** - Time (milliseconds) that the alarm will refuse arm/disarm attempts after an intruder lockout trigger.
- **INTRUDER_LOCKOUT_THRESHOLD** - Maximum unsuccessful login attempts before intruder lockout kicks in.
- **SIREN_CUTOFF_TIME** - Time (ms) that the siren will remain on for after an alarm is triggered. **Set to 2 min by default - you will want to increase this.** This *includes* the E/E time so take that into account.

All buzzer frequencies and durations can also be adjusted - please refer to the "tone()" documentation in order to do so. In cases where the "tone(a, b, c)" command is followed by "delay(c)", the duration must be adjusted in both the tone and delay commands.

## Features
- MIFARE RFID authentication for arming, disarming, and resetting.
- Entry/exit delay
- 4 zones, with the capability for more or fewer if desired.
- Relay siren output
- Multi-tone buzzer for audible feedback
- LCD display for important messages
- Configurable intruder lockout to prevent brute-forcing of RFID IDs
- Cards can be added/removed by scanning an admin card while an admin button is pushed.
- Siren timeout - when an alarm is triggered, the siren can be switched off after a set amount of time to avoid noise disturbances.

## Operation
### Arming/Disarming
When "Please scan card" appears, simply scan a card to arm or disarm. When arming, the exit delay will occur before the arm is applied.


### Alarms
If an non-entry zone (i.e. where **DELAY_ENABLE** is set to false) is triggered while armed, the siren will immediately zound and "Alarm Z_" will be displayed. Scan a valid card to silence/reset the alarm and disarm the system.

If an entry zone is triggered, the entry delay will first occur - the system will beep, increasing its frequency towards the end of the delay period. After the delay, the alarm will sound exactly as above.

In both cases, the audible siren will time out - however the alarm still must be reset.

### Adding/removing cards
This can only be done by the master user. *Note: No buzzer sounds in this mode just yet.*
1. In arm/disarm state, hold down the admin button and scan the admin card (i.e. the one assigned to user ID 0). Please note that if a non-admin card is used, arming/disarming will commence as normal.
2. Release the button immediately after scanning the card. You should see "Admin mode: Push to exit" followed by "Scan to add; Scan to remove" - if it says "Please scan card", you have held the button down too long.

At this point, scan a card:
- If the card is the admin card, an error will be displayed as the admin card cannot be removed.
- If the card is already saved, it will be removed and its user ID will be used for the next-added user.
- If the card is not recognised, it will be added - unless the memory is full (around 146 card stored), in which case an error is displayed and an existing card must be removed before adding a new one.

To leave admin mode, press the button again. Ensure that the display shows "Please scan card" before arming, disarming, or resetting alarms.


## Future additions
- Add the option of only being able to reset the alarm with a designated admin card.
- Record every zone triggered during an alarm - not just the first in the array.
- Log alarm events to EEPROM, SD, etc.
- Add some 24/7 active zones for tamper and/or panic buttons.
- Only allow card modification in disarm state.
- Allow other events while an entry delay is taking place.
