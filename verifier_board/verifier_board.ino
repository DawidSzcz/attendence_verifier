#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <UsbFat.h>
#include <DS1307RTC.h>


#define FILENAME_FORMAT "%02d-%02d-%02d-%02d_%02d_%02d"
#define TIME_FORMAT "%02d/%02d %02d:%02d:%02d"

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
#define MAX_TONE 6000
#define TONE_STEP 25
#define DEBUG false


#define LOOP_DELAY 500

USB usb;
BulkOnly bulk(&usb);
UsbFat key(&bulk);
bool usb_inserted = false;
char file_name[25];
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
LiquidCrystal lcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN); // initialize the lcd
File file_ptr;

void setup() 
{
    Serial.begin(9600);   // Initiate a serial communication 
    while (!Serial); 
    #if DEBUG
        Serial.println("SETUP");
    #endif
    
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() 
{       
    if (!usb_inserted) {
        if (!initUSB(&usb)) {
            #if DEBUG
                Serial.println("initUSB fail");
            #endif
            printLCD("Insert pendrive", "");
        } else if(!key.begin()) {
            #if DEBUG
                Serial.println("key.begin fail");
            #endif
            printLCD("Initialization", "Error");
            looped();
        } else {
            onUsbIn();
        }
    } else {
        #if DEBUG
            Serial.println("State: Inserted");
        #endif
        if (!key.init()) {
            onUsbOut();
        } else if (mfrc522.PICC_IsNewCardPresent()) {
            #if DEBUG
                Serial.println("PICC_IsNewCardPresent success");
            #endif
            if ( !mfrc522.PICC_ReadCardSerial()) {
                #if DEBUG
                    Serial.println("Fail read card");
                #endif
                printLCD("Read card error", "");
                delay(1000);
            } else {
                onCardRead();
            }
        } else {
            printLCD("Scan card", "");      
        }
    }
    delay(LOOP_DELAY);
}

void onCardRead() {  
    String content = readRfidUid();

    if (!file_ptr.open(file_name, O_CREAT | O_RDWR | O_APPEND)
      || !file_ptr.println(content)) {
        #if DEBUG
            Serial.println("open file fail");
        #endif
        printLCD("Open file error", "Reinsert USB");
        while(key.init()){}
        return;
    }
    file_ptr.close();
  
    #if DEBUG
        Serial.println(content + " Printed");
    #endif
    printLCD("Read Success", ""); 
    buzzer();
}

void onUsbOut() {
    #if DEBUG
        Serial.println("USB Out");
    #endif
    usb_inserted = false;
}

void onUsbIn() {
    #if DEBUG
        Serial.println("USB Inserted");
    #endif
    printLCD("USB Inserted", "");
    delay(1000);

    #if DEBUG
        Serial.println("createFileName");
    #endif
    tmElements_t tm = getTime();
    sprintf(file_name, FILENAME_FORMAT, tm.Day, tm.Month, tmYearToCalendar(tm.Year),tm.Hour, tm.Minute, tm.Second);

    usb_inserted = true;
    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
}

tmElements_t getTime() {
    tmElements_t tm;
    if (RTC.read(tm)) {
        #if DEBUG
            Serial.println("RTC.read success");
        #endif
    } else {
        #if DEBUG
            Serial.println("time fail");
        #endif
        printLCD("Clock Error. Set", "time to continue");
        looped();
    }

    return tm;
}

String readRfidUid()
{
    #if DEBUG
        Serial.println("ReadRfidUid");
    #endif
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
    #if DEBUG
        Serial.println("Buzzer");
    #endif
    for(int i = 0; i < BUZZ_TIME; i += TONE_STEP) {
        noTone(BUZZER_PIN);
        tone(BUZZER_PIN, i, MAX_TONE * TONE_STEP / BUZZ_TIME);
        delay(TONE_STEP);
    }
}

void printLCD(char* content1, char* content2)
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print(content1);
    lcd.setCursor(0, 1);
    
    if(content2[0] == 0) {
        char buff[20];
        tmElements_t tm = getTime();
        
        sprintf(buff, TIME_FORMAT, tm.Day, tm.Month,tm.Hour, tm.Minute, tm.Second);
        lcd.print(buff);
    } else {
        lcd.print(content2);
    }
}
