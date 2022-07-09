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
- 10K resistor
- Momentary push button
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

### Admin button

A button is used to add and remove cards. This is connected to **digital pin 7** and is wired using a 10K pull-down resistor - the same as in the Arduino button wiring guide.

## Configurable Constants etc

Anything in this code can obviously be changed, but as for the most important options:

- **POLLING_RATE**: This is the polling rate for alarm and RFID inputs in Hz. It is 10 by default but can be modified if you feel the need.
- **-PIN constants** - Any pins can be changed, so long as the newly chosen pins are still suitable.
- **ZONE_PINS** - You can add as many pins as you like to this - just make sure you update ZONE\_QTY.
- **EE_TIME** - Entry/exit delay time in *milliseconds*
- **INTRUDER_LOCKOUT_TIME** - Time (milliseconds) that the alarm will refuse arm/disarm attempts after an intruder lockout trigger.
- **INTRUDER_LOCKOUT_THRESHOLD** - Maximum unsuccessful login attempts before intruder lockout kicks in.

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

## Operation
### Arming/Disarming
When "Please scan card" appears, simply scan a card to arm or disarm. When arming, the exit delay will occur before the arm is applied.

### Entry delay
When a sensor is triggered, the entry delay (beeping which speeds up in the last few seconds) will start. During this time, it can be disarmed by scanning a valid card. If this is not done, the alarm will be triggered.

### Alarms
When the entry delay runs out, a message will appear showing the alarm trigger and the siren will be activated - though the buzzer will be silent. Scan a valid card to silence/reset the alarm - this will also disarm the system.

### Adding/removing cards
This can only be done by the master user. *Note: No buzzer sounds in this mode just yet.*
1. In arm/disarm state, hold down the admin button and scan the admin card (i.e. the one assigned to user ID 0). Please note that if a non-admin card is used, nothing will happen - no error will be displayed.
2. Release the button immediately after scanning the card. You should see "Admin mode: Push to exit" followed by "Scan to add; Scan to remove" - if it says "Please scan card", you have held the button down too long.

At this point, scan a card:
- If the card is the admin card, an error will be displayed as the admin card cannot be removed.
- If the card is already saved, it will be removed and its user ID will be used for the next-added user.
- If the card is not recognised, it will be added - unless the memory is full (around 146 card stored), in which case an error is displayed and an existing card must be removed before adding a new one.

To leave admin mode, press the button again. Ensure that the display shows "Please scan card" before arming, disarming, or resetting alarms.


## Future additions
- Make the alarm time out so it doesn't go on forever if no card is scanned.
- Add the option of only being able to reset the alarm with a designated admin card.
- Exclude certain zones from E/E delays
- Record every zone triggered during an alarm - not just the first in the array.
- Log alarm events to EEPROM, SD, etc.
- Add some 24/7 active zones for tamper and/or panic buttons.
- Only allow card modification in disarm state.
