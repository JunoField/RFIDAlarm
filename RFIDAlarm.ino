#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>

//RFID constants
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int USER_QTY = 2;
String ids[USER_QTY] = { " 07 D0 E4 A7", " 04 6E 5F 7A 8E 6D 80" }; //MUST add additional space at the start of ID!
String idNames[USER_QTY] = { "Dave", "Juno" }; //names for each id
String lastId = ""; //stores last scanned ID, must be reset to NOT_PRESENT after use.



//Alarm constants
boolean armStatus = false; 
boolean alarmStatus = false;
const double POLLING_RATE = 10; //polling rate in Hz.
const int SIREN_PIN  = 8;
const int ZONE_QTY = 4;
const int ZONE_PINS[ZONE_QTY] = { 2, 3, 4, 5 }; //list of zone input pins
const int BUZZER_PIN = 7;


//Get ID from scanned card
String scanCardGetId(MFRC522 mfrc522){
  if (!mfrc522.PICC_IsNewCardPresent()){
    return "NOT_PRESENT";
  }
  if (!mfrc522.PICC_ReadCardSerial()){
    return "NOT_READABLE";
  }
  //if card is present:
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++){
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content;
}

//check that given ID is in ids list
int authenticate(String id){
	for (int i = 0; i < USER_QTY; i++){
		if (ids[i].equals(id)){
			return i;	
		}
	}
	return -1; //if id is not in array
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

void exitDelay(int time){
	time = time / 8;
	int freq = 523;
	for (int i = 0; i < 8; i++){
		tone(BUZZER_PIN, freq, time);
		freq += 64;
		delay(time);
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
	lastId = scanCardGetId(mfrc522);
	if (!lastId.equals("NOT_PRESENT")){
		if (authenticate(lastId) >= 0){
			if (armStatus){
				disarm(idNames[authenticate(lastId)]);
			} else{
				arm(idNames[authenticate(lastId)]);
			}
		} else{
			printToLCD("Incorrect card", "Access denied");
     			tone(BUZZER_PIN, 800, 150);
    			delay(150);
			printToLCD("Please scan card");
			tone(BUZZER_PIN, 400, 850);

		}
	}
	
	//add sensor input processing below:
	int zoneNo = querySensors(); //if none returns -1
	if (!(zoneNo== -1) && armStatus){
		alarm(zoneNo);
	}
}


void disarm(String userName){
	armStatus = false;
	printToLCD("Disarmed", "Hello " + userName);
	tone(BUZZER_PIN, 850, 150);
	delay(150);
	tone(BUZZER_PIN, 1200, 300);
	delay(850);
	printToLCD("Please scan card");
}

void arm(String userName){
	printToLCD("Hello " + userName, "Arming - LEAVE");
	exitDelay(10000);
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


void alarm(int source){
	alarmStatus = true;
	printToLCD("Entry delay", "Scan card NOW");
	unsigned long timeTriggered = millis();
	while (alarmStatus && millis() - timeTriggered < 10000){
		if ((millis() - timeTriggered) % 1000 < 50){
			tone(BUZZER_PIN, 523, 250);
		}
		lastId = scanCardGetId(mfrc522);
		if (authenticate(lastId) >= 0){
			printToLCD("SCANNED");
			alarmStatus = false;
		} else if (!lastId.equals("NOT_PRESENT")){
			printToLCD("Incorrect card", "Warning ALARM!");
		}
	}


	printToLCD("Alarm Z" + String(source), "Scan to reset");
	digitalWrite(SIREN_PIN, HIGH);
	while(alarmStatus){
		lastId = scanCardGetId(mfrc522);
		if (authenticate(lastId) >= 0){
			alarmStatus = false;
		} else if (!lastId.equals("NOT_PRESENT")){
			printToLCD("Alarm Z" + String(source), "Incorrect card");
		}
	}
	digitalWrite(SIREN_PIN, LOW);	
	printToLCD("Alarm reset");
	delay(1000);
	disarm(idNames[authenticate(lastId)]);
	
}
