#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <UsbFat.h>
#include <DS1307RTC.h>


#define FILENAME_FORMAT "%d_%d_%d_%d%d%d.txt"

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


#define LOOP_DELAY 500

USB usb;
BulkOnly bulk(&usb);
UsbFat key(&bulk);
bool usb_inserted = false;
File file_ptr;
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
LiquidCrystal lcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN); // initialize the lcd



void setup() 
{
    Serial.begin(9600);   // Initiate a serial communication 
    while (!Serial); 
    
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522

    if (!initUSB(&usb)) {
        Serial.println("initUSB fail");
        printLCD("Insert pendrive", 0);
    } else if(!key.begin()) {
        Serial.println("key.begin fail");
        printLCD("Initialization Error", 0);
        looped();
    } else {
        onUsbIn();
    }
}

void loop() 
{   
    if (!usb_inserted) {
        Serial.println("State: not Inserted");
        if (initUSB(&usb)) {
            if(!key.begin()) {
                Serial.println("key.begin fail");
                printLCD("Initialization Error", 0);
                looped();
            } else {
                onUsbIn();
            }
        }
    } else {
        Serial.println("State: Inserted"); 
        if (!key.init()) {
            onUsbOut();
        } else {
            if (mfrc522.PICC_IsNewCardPresent()) {
                Serial.println("PICC_IsNewCardPresent success");
                if ( !mfrc522.PICC_ReadCardSerial()) {
                    Serial.println("Fail read card");
                    printLCD("Read Cart error", 0);
                    delay(1000);
                    printLCD("Scan card", 0);
                } else {
                    onCardRead();
                }
            }     
        }
    }
    delay(LOOP_DELAY);
}

void onCardRead() {
  String content = readRfidUid();
  file_ptr.println(content);   
  Serial.println(content + " Printed");
  printLCD("Read Success" , 0); 
  buzzer();
  printLCD("Scan Card" , 0);   
}

void onUsbOut() {
    Serial.println("USB Out");
    file_ptr.close();
    printLCD("Inseert Pendrive", 0);
    printLCD("", 1);
    usb_inserted = false;
}

void onUsbIn() {
    Serial.println("USB Inserted");
    if(!createFile()) {
        Serial.println("open file fail");
        printLCD("Reinsert pendrive", 0);
        looped();
    } else {
        printLCD("USB Inserted", 0);
        usb_inserted = true;
        delay(1000);
        printLCD("Scan Card", 0);
    }
}


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

void looped()
{
  while(true){};
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

bool createFile()
{
    Serial.println("createFile");
    tmElements_t tm;
  
    if (RTC.read(tm)) {
        Serial.println("RTC.read success");
        char buff[25];
        sprintf(buff, FILENAME_FORMAT, tm.Day, tm.Month, tmYearToCalendar(tm.Year),tm.Hour, tm.Minute, tm.Minute);
        Serial.println(buff);
    
        if(file_ptr.open(buff, O_CREAT | O_RDWR | O_APPEND)) {
            printLCD(buff, 1);
            return true;
        }
    }

    return false;
}

void printLCD(char* content, int line)
{
  lcd.begin(16, 2);
  lcd.setCursor(0, line);
  lcd.print(content);
}