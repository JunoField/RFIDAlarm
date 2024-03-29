#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>

//MFRC522 RFID reader
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

//LCD output
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Card IDs
const int ID_BITS = 7;
byte lastId[ID_BITS]; //variable for last present ID

//Alarm variables 
boolean armStatus = false; 
boolean alarmStatus = false;
int incorrectLoginAttempts = 0;

//Alarm constants
const double POLLING_RATE = 10; //polling rate in Hz.
const int SIREN_PIN = 8; //siren relay pin
const int ADMIN_BUTTON_PIN = 6;
const int ZONE_QTY = 4; //number of zone inputs
const int ZONE_PINS[ZONE_QTY] = { 2, 3, 4, 5 }; //list of zone input pins
const int DELAY_ENABLE[ZONE_QTY] = {true, false, false, false}; //delay status for each pin, true for entry zones
const int BUZZER_PIN = 7; //digital pin for buzzer output
const unsigned long EE_TIME = 10000; //entry-exit delay time in milliseconds
const unsigned long INTRUDER_LOCKOUT_TIME = 300000; //intruder lockout time in ms
const int INTRUDER_LOCKOUT_THRESHOLD = 6; //no of incorrect logins for lockout to start
const unsigned long SIREN_CUTOFF_TIME = 120000; //time that siren will remain on during an alarm.


//Updated - get card id into lastId var and return true if card exists, false otherwise.
boolean getCardId(){
	if (!mfrc522.PICC_IsNewCardPresent()){
		return 0;
	}
	if (!mfrc522.PICC_ReadCardSerial()){
		return 0;
	}
	//should work with 4 and 7 bit UIDs
	for (int i = 0; i < ID_BITS; i++){
		lastId[i] = mfrc522.uid.uidByte[i];
		mfrc522.uid.uidByte[i] = 0x00; //reset all to 0 to avoid causing problems with 4/7 bit cards
	}
	return 1;
}


//Updated - check if two  IDs match
boolean checkMatch(byte a[], byte b[]){
	for (int i = 0; i < ID_BITS; i++){
		if (a[i] != b[i]){
			return false;
		}
	}
	return true;
}




//Check if the current ID is in EEPROM:
int authenticateCard(){
	int eepromAddr = 0; //stores current ID start address
	byte readID [ID_BITS]; //stores last id read from eeprom
	while (eepromAddr < 1017){
		for (int i = 0; i < ID_BITS; i++){ //go thru each byte in that id and store as the last read id from eeprom.
			readID[i] = EEPROM.read(eepromAddr + i);
		}
		if (checkMatch(readID, lastId)){
			return eepromAddr / 7; //if found, return its user index (not its eeprom address as of yet)
		}
		eepromAddr += 7;
	}
	return -1;
	
}

int findEmptyID(){
	int idSum = 0;
	for (int i = 0; i < 1017; i += 7){ 
		for (int j = 0; j < ID_BITS; j++){ //get sum of ID - if this is zero, then the entire slot is clear.
			idSum += EEPROM.read(i + j);
		}
		if (idSum == 0){ //if sum of ID is 0, then we will use that slot.
			return i;
		}
		idSum = 0;
	}
	return -1; //if no space, return -1
}







//print a line or 2 lines to LCD
void printToLCD(String line1, String line2){
  if ((line1.length() > 16) || (line2.length() > 16)){
    printToLCD("Runtime Error", "String too long");
    return;
  }
  else{
    lcd.clear();
    lcd.setCursor(((16 - line1.length()) / 2), 0);
    lcd.print(line1);
    lcd.setCursor(((16 - line2.length()) / 2), 1);
    lcd.print(line2);
  }
}
void printToLCD(String text){
  if (text.length() < 16){
    printToLCD(text, "");
  }
  else{
    printToLCD(text.substring(0, 16), text.substring(16));
  }
 
}


//Adds a new card to EEPROM (triggered by using master key)
void modifyCards(){
	printToLCD("Admin Mode", "Push to exit");
	delay(500);
	printToLCD("Scan to add", "Scan to remove");
	while (digitalRead(ADMIN_BUTTON_PIN) == LOW){
		if (getCardId() == 1){ //when a card is present:	
			if (authenticateCard() == 0){ //throw error if master card is scanned again
				printToLCD("Error: Cannot", "remove master");
			} else if (authenticateCard() == -1){ //if card doesn't exist, add it.
				int newSlot = findEmptyID();
				if (newSlot == -1){
					printToLCD("Memory full", "Remove cards 1st");
				} else {
					for (int i = 0; i < ID_BITS; i++){
						EEPROM.write(newSlot + i, lastId[i]);
					}
					printToLCD("Added card", "User no. " + String(newSlot / 7));
				}
			} else { //if card does exist, remove it.
				int userNo = authenticateCard();
				for (int i = userNo * 7; i < ((userNo * 7) + 7); i++){
					EEPROM.write(i, 0x00);
				}
				printToLCD("Removed card", "user ID " + String(userNo));
				delay(500);
				printToLCD("Scan to add", "Scan to remove");
			}
			delay(500);
			printToLCD("Scan to add", "Scan to remove");
		}
	}
	printToLCD("Please scan card"); //back to normal mode
}


//exit delay
void exitDelay(){
	int freq = 523;
	for (int i = 0; i < 8; i++){
		tone(BUZZER_PIN, freq, (EE_TIME / 8));
		freq += 64;
		delay(EE_TIME / 8);
	}
}

//Intruder Lockout
void intruderLockout(){
	int timeLockedOut = millis();
	while (millis() - timeLockedOut < INTRUDER_LOCKOUT_TIME){
		printToLCD("Intruder Lockout", "WAIT " + String((INTRUDER_LOCKOUT_TIME - millis() + timeLockedOut) / 1000));
		tone(BUZZER_PIN, 122, 250);
		delay(250);
		tone(BUZZER_PIN, 346, 250);
		delay(250);
		tone(BUZZER_PIN, 909, 250);
		delay(250);
		tone(BUZZER_PIN, 3200, 250);
		delay(250);
	}	
}


void setup(){
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.clear();  
  printToLCD("RFID Alarm", "J Field");
  delay(2000);
	printToLCD("Please scan card");
	pinMode(SIREN_PIN, OUTPUT);
	digitalWrite(SIREN_PIN, LOW);
	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(ADMIN_BUTTON_PIN, INPUT);
	for (int i = 0; i < ZONE_QTY; i++){
		pinMode(i, INPUT_PULLUP);
	}     
}


void loop(){
	delay(1000 / POLLING_RATE);
	if (getCardId()){ //if card is present on reader:
		if ((authenticateCard() == 0) && (digitalRead(ADMIN_BUTTON_PIN) == HIGH)){ //if master card is scanned and button is held down
			modifyCards();
		}	
		else if (authenticateCard() >= 0){ //if card is in array:
			incorrectLoginAttempts = 0;
			if (armStatus){
				disarm(authenticateCard());
			} else{
				arm(authenticateCard());
			}
		} else{
			printToLCD("Incorrect card", "Access denied");
     			tone(BUZZER_PIN, 800, 150);
    			delay(150);
			tone(BUZZER_PIN, 400, 850);
			delay(850);
			incorrectLoginAttempts++;
			if (incorrectLoginAttempts > INTRUDER_LOCKOUT_THRESHOLD){
				intruderLockout();
			}
			printToLCD("Please scan card");
		}
	}
	
	//query sensors for alarm
    if (armStatus){
        querySensors();
    }
}


//Disarms alarm
void disarm(int userNo){
	armStatus = false;
	printToLCD("Disarmed", "Hello U" + String(userNo));
	tone(BUZZER_PIN, 850, 150);
	delay(150);
	tone(BUZZER_PIN, 1200, 300);
	delay(850);
	printToLCD("Please scan card");
}

//Arms alarm
void arm(int userNo){
	printToLCD("Hello U" + String(userNo), "Arming - LEAVE");
	exitDelay();
	armStatus = true;
	printToLCD("Armed", "Scan to disarm");
	delay(1000);
	printToLCD("Please scan card");
}

//gets index of alarmed sensor, otherwise returns -1
void querySensors(){
    //check instant zones first:
    for (int i = 0; i < ZONE_QTY; i++){
        if (digitalRead(ZONE_PINS[i]) == HIGH && !DELAY_ENABLE[i]) {
            alarmNow(i);
            return;
        }
    }

    //then include delay zones too:
	for (int i = 0; i < ZONE_QTY; i++){
		if (digitalRead(ZONE_PINS[i]) == HIGH){
			alarmWithDelay(i);
            return;
		}
	}
}


void alarmNow(int source){
	alarmStatus = true;
	unsigned long timeTriggered = millis();

	//Alarm 
	printToLCD("Alarm Z" + String(source), "Scan to reset");
	boolean sirenOn = true;
	digitalWrite(SIREN_PIN, HIGH);
	while(alarmStatus){
		if (getCardId()){
			if (authenticateCard() >= 0){
				alarmStatus = false;
				printToLCD("Alarm reset");
				delay(1000);
			} else{
				printToLCD("Alarm Z" + String(source), "Incorrect Card");
     				tone(BUZZER_PIN, 800, 150);
    				delay(150);
				tone(BUZZER_PIN, 400, 850);
			}
		}

		if (sirenOn && (millis() > (timeTriggered + SIREN_CUTOFF_TIME))){ //switch off siren after a set amount of time
			digitalWrite(SIREN_PIN, LOW);
			sirenOn = false;
			printToLCD("Alarm Z" + String(source), "Silenced");
		}
	}
	digitalWrite(SIREN_PIN, LOW);
	disarm(authenticateCard());
}


//Runs when a sensor is triggered
void alarmWithDelay(int source){
	alarmStatus = true;
	printToLCD("Entry delay", "Scan card NOW");
	unsigned long timeTriggered = millis();
	//Entry delay:
	while (alarmStatus && millis() - timeTriggered < EE_TIME){
		//Beep:
		if ((millis() - timeTriggered) % 1000 < 50 && (millis() - timeTriggered) < (EE_TIME * 0.75)){
			tone(BUZZER_PIN, 523, 250);
		} else if ((millis() - timeTriggered) % 300 < 50 && (millis() - timeTriggered) > (EE_TIME * 0.75)){
			tone(BUZZER_PIN, 743, 150);
		}
		
		//Authentication:
		if (getCardId()){
			if (authenticateCard() >= 0){
				alarmStatus = false;
			} else{
				printToLCD("Incorrect Card", "Warning ALARM!");
     				tone(BUZZER_PIN, 800, 150);
    				delay(150);
				tone(BUZZER_PIN, 400, 850);
			}
		}
	}

	//Alarm (after entry delay is over)
	printToLCD("Alarm Z" + String(source), "Scan to reset");
	boolean sirenOn = true;
	digitalWrite(SIREN_PIN, HIGH);
	while(alarmStatus){
		if (getCardId()){
			if (authenticateCard() >= 0){
				alarmStatus = false;
				printToLCD("Alarm reset");
				delay(1000);
			} else{
				printToLCD("Alarm Z" + String(source), "Incorrect Card");
     				tone(BUZZER_PIN, 800, 150);
    				delay(150);
				tone(BUZZER_PIN, 400, 850);
			}
		}
		
		if (sirenOn && (millis() > (timeTriggered + SIREN_CUTOFF_TIME))){ //switch off siren after a set amount of time
			digitalWrite(SIREN_PIN, LOW);
			sirenOn = false;
			printToLCD("Alarm Z" + String(source), "Silenced");	
		}
	}
	digitalWrite(SIREN_PIN, LOW);	
	disarm(authenticateCard());
}
