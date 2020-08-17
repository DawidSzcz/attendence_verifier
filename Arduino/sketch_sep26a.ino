#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <UsbFat.h>
#include <masstorage.h>

#define FILE_PREFIX "attenders"
#define FILE_TYPE "txt"

/*
 * Pin 13 SCK
 * Pin 12 MISO
 * Pin 11 MOSI
 */ 
#define SS_PIN 8
#define RST_PIN 9
#define BUZZER_PIN A1

#define LCD_RESET_PIN 7
#define LCD_ENABLE_PIN 6
#define LCD_D4_PIN 5
#define LCD_D5_PIN 4
#define LCD_D6_PIN 3
#define LCD_D7_PIN 2

#define BUZZ_TIME 1000
#define DELAY_TIME 3000
#define MAX_TONE 6000
#define TONE_STEP 25


#define INSERTED_DELAY 1000

USB usb;
BulkOnly bulk(&usb);
UsbFat key(&bulk);
bool usb_inserted = false;
char file_name[20];

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// initialize the lcd
LiquidCrystal lcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

String readRfidUid()
{
  Serial.println("ReadRfidUid");
  String content = "";
  content.concat(String(mfrc522.uid.uidByte[0] < 0x10 ? "0" : ""));
  content.concat(String(mfrc522.uid.uidByte[0], HEX));
  for (byte i = 1; i < mfrc522.uid.size; i++) 
  {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  return content;
}

void writeUid(String uid)
{  
  Serial.println("writeUid");
  File file; 
    
  delay(10); 
  
  file.open(file_name, O_CREAT | O_RDWR | O_APPEND);
  file.println(uid);
  file.close();
  
  Serial.println(uid + " Printed");
}

void buzzer()
{
  Serial.println("Buzzer");
  for(int i = 0; i < BUZZ_TIME; i += TONE_STEP) {
    noTone(BUZZER_PIN);
    tone(BUZZER_PIN, i, MAX_TONE * TONE_STEP / BUZZ_TIME);
    delay(TONE_STEP);
  }
  delay(DELAY_TIME);
}

void createFileName()
{
  for(int i = 0; i < 100; i++) {
    String temp = (String)FILE_PREFIX +  (i > 0 ? "_" + (String)i : "") + "." + (String)FILE_TYPE;
    strcpy(file_name, temp.c_str());
    if(!key.exists(file_name)) {
      return;
    }
  }
}
void setup() 
{
  Serial.begin(9600);   // Initiate a serial communication 
  while (!Serial); 
  Serial.println("SETUP");
  
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, LOW);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if (!initUSB(&usb)) {
    Serial.println(F("initUSB failed"));
    return;
  } else {
    Serial.println("initUSB success");
  }
  if (!key.begin()) {
    Serial.println(F("key.begin failed"));
    return;
  } else {
    Serial.println("key.begin success");
  }
  usb_inserted = true;
  createFileName();

  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  delay(4); 
}

void printLCD(String content, int line)
{
  lcd.begin(16, 2);
  lcd.setCursor(0, line);
  lcd.print(content);
}
void loop() 
{    
  if (usb_inserted) {
    if (!key.init()) {
      //usb just pulled out 
      Serial.println("usb just pulled out");
      
      usb_inserted = false;
      return;     
    } else {
      Serial.println("Waiting for card");
      printLCD("Scan card", 0);
    }
  } else {
    if(initUSB(&usb)) {
       if (!key.begin()) {
          Serial.println(F("key.begin failed"));
          printLCD("USB Init fail", 0);
          return;
        } else {
          Serial.println("key.begin success");
          printLCD("USB Inserted", 0);
          usb_inserted = true;
          createFileName();

          SPI.begin();      // Initiate  SPI bus
          mfrc522.PCD_Init();   // Initiate MFRC522

          delay(INSERTED_DELAY);
        }
    } else {
      printLCD("Insert USB", 0);
      return;
    }
  }
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    Serial.println("Fail new card");
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    Serial.println("Fail read card");
    return;
  }
  
  String content = readRfidUid();

  printLCD("Card UID:" , 0); 
  printLCD(content, 1); 
  writeUid(content);   
  buzzer();
} 
