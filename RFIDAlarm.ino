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
const int USER_QTY = 2;
byte ids[USER_QTY][4] = {
				{0x07, 0xD0, 0xE4, 0xA7},
				{0x04, 0x6E, 0x5F, 0x7A} //remaining bytes 8E 6D 80}
			};
String idNames[USER_QTY] = { "Dave", "Juno" }; //names for each id
byte lastId[4]; //variable for last present ID

//Alarm variables
boolean armStatus = false; 
boolean alarmStatus = false;

//Alarm constants
const double POLLING_RATE = 10; //polling rate in Hz.
const int SIREN_PIN  = 8; //siren relay pin
const int ZONE_QTY = 4; //number of zone inputs
const int ZONE_PINS[ZONE_QTY] = { 2, 3, 4, 5 }; //list of zone input pins
const int BUZZER_PIN = 7; //digital pin for buzzer output
const int EE_TIME = 10000; //entry-exit delay time in milliseconds


//Updated - get card id into lastId var and return true if card exists, false otherwise.
boolean getCardId(){
	if (!mfrc522.PICC_IsNewCardPresent()){
		return 0;
	}
	if (!mfrc522.PICC_ReadCardSerial()){
		return 0;
	}
	//assume 4 byte UID
	for (int i = 0; i < 4; i++){
		lastId[i] = mfrc522.uid.uidByte[i];
	}
	return 1;
}

//Updated = check if two 4-byte IDs match
boolean checkMatch(byte a[], byte b[]){
	for (int i = 0; i < 4; i++){
		if (a[i] != b[i]){
			return false;
		}
	}
	return true;
}


//Updated - check if current ID is in the array
int authenticateCard(){
	for (int i = 0; i < USER_QTY; i++){
		if (checkMatch(ids[i], lastId)){
			return i;
		}
	}
	//if not found:
	return -1;
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


//exit delay
void exitDelay(){
	int freq = 523;
	for (int i = 0; i < 8; i++){
		tone(BUZZER_PIN, freq, (EE_TIME / 8));
		freq += 64;
		delay(EE_TIME / 8);
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
	for (int i = 0; i < ZONE_QTY; i++){
		pinMode(i, INPUT_PULLUP);
	}     
}


void loop(){
	delay(1000 / POLLING_RATE);
	if (getCardId()){ //if card is rpesent on reader:
		if (authenticateCard() >= 0){ //if card is in array:
			if (armStatus){
				disarm(idNames[authenticateCard()]);
			} else{
				arm(idNames[authenticateCard()]);
			}
		} else{
			printToLCD("Incorrect card", "Access denied");
     			tone(BUZZER_PIN, 800, 150);
    			delay(150);
			tone(BUZZER_PIN, 400, 850);
			delay(850);
			printToLCD("Please scan card");
		}
	}
	
	//query sensors for alarm
	int zoneNo = querySensors(); //if none returns -1
	if (!(zoneNo== -1) && armStatus){
		alarm(zoneNo);
	}
}


//Disarms alarm
void disarm(String userName){
	armStatus = false;
	printToLCD("Disarmed", "Hello " + userName);
	tone(BUZZER_PIN, 850, 150);
	delay(150);
	tone(BUZZER_PIN, 1200, 300);
	delay(850);
	printToLCD("Please scan card");
}

//Arms alarm
void arm(String userName){
	printToLCD("Hello " + userName, "Arming - LEAVE");
	exitDelay();
	armStatus = true;
	printToLCD("Armed", "Scan to disarm");
	delay(1000);
	printToLCD("Please scan card");
}

//gets index of alarmed sensor, otherwise returns -1
int querySensors(){
	for (int i = 0; i < ZONE_QTY; i++){
		if (digitalRead(ZONE_PINS[i]) == HIGH){
			return i;
		}
	}
	return -1; //if none found, return -1
}


//Runs when a sensor is triggered
void alarm(int source){
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
	}
	digitalWrite(SIREN_PIN, LOW);	
	disarm(idNames[authenticateCard()]);
}
