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
String adminId = " 07 D0 E4 A7"; //MUST add additional space at the start of ID!
String lastId = ""; //stores last scanned ID, must be reset to NOT_PRESENT after use.



//Alarm constants
boolean armStatus = false; 
const double POLLING_RATE = 10; //polling rate in Hz.
const int SIREN_PIN  = 8;

const int SENSOR_PIN = 2;


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
	pinMode(SENSOR_PIN, INPUT_PULLUP);
	digitalWrite(SIREN_PIN, LOW);
}

void loop(){
	delay(1000 / POLLING_RATE);
	String lastId = scanCardGetId(mfrc522);
	if (!lastId.equals("NOT_PRESENT")){
		if (lastId.equals(adminId)){
			if (armStatus){
				disarm();
			} else{
				arm();
			}
		} else{
			printToLCD("Incorrect card");
		}
	}
	
	//add sensor input processing below:
	if (digitalRead(SENSOR_PIN) == HIGH){
		processSensorInput();
	}
	
}

void disarm(){
	armStatus = false;
	printToLCD("Disarmed");
	delay(1000);
	printToLCD("Please scan card");
}

void arm(){
	armStatus = true;
	printToLCD("Armed");
	delay(1000);
	printToLCD("Please scan card");
}

void processSensorInput(){
	if (armStatus){
		alarm();
	}
}



void alarm(){
	printToLCD("Alarm", "Scan to reset");
	boolean alarmStatus = true;
	digitalWrite(SIREN_PIN, HIGH);
	while(alarmStatus){
		lastId = scanCardGetId(mfrc522);
		if (lastId.equals(adminId)){
			alarmStatus = false;
		} else if (!lastId.equals("NOT_PRESENT")){
			printToLCD("Incorrect Card");
		}
	}
	digitalWrite(SIREN_PIN, LOW);	
	printToLCD("Alarm reset");
	delay(1000);
	disarm();
	
}
