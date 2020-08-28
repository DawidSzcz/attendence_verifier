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

#define STATE_USB_WAIT 1
#define STATE_CARD_WAIT 2
#define STATE_CLOCK_ERROR 3
#define STATE_INIT_ERROR 4
#define STATE_USB_FOUND 5
#define STATE_USB_LOST 6
#define STATE_CARD_FOUND 7
#define STATE_FILE_ERROR 8
int state = STATE_USB_WAIT;

#define LOOP_DELAY 100

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
    tmElements_t tm = getTime();
    
    switch(state) {
        case STATE_USB_WAIT:
            stateUsbWait(tm);
            break;
        case STATE_CARD_WAIT:
            stateCardWait(tm);
            break;
        case STATE_CLOCK_ERROR:
            stateClockError(tm);
            break;
        case STATE_INIT_ERROR:
            stateInitError(tm);
            break;
        case STATE_FILE_ERROR:
            stateFileError(tm);
            break;
        case STATE_USB_FOUND:
            stateUsbFound(tm);
            break;
        case STATE_CARD_FOUND:
            stateCardFound(tm);
            break;
        case STATE_USB_LOST:
            stateUsbLost();
            break;
            
    }
    delay(LOOP_DELAY);
}

void stateUsbWait(tmElements_t tm)
{
    #if DEBUG 
        Serial.println("stateUsb"); 
    #endif
    printLCD("Insert pendrive", "", tm);
    if (!initUSB(&usb)) {
        #if DEBUG 
            Serial.println("initUSB fail"); 
        #endif
    } else if(!key.begin()) {
        #if DEBUG 
            Serial.println("key.begin fail"); 
        #endif
        state = STATE_INIT_ERROR;
    } else {    
        #if DEBUG  
            Serial.println("USB Inserted"); 
        #endif
        state = STATE_USB_FOUND;
    }
}

void stateCardWait(tmElements_t tm)
{
    printLCD("Scan card", "", tm);   
    #if DEBUG 
        Serial.println("stateCard"); 
    #endif
    
    if (!key.init()) {
        state = STATE_USB_LOST;
    } else if (mfrc522.PICC_IsNewCardPresent()) {
        #if DEBUG 
            Serial.println("PICC_IsNewCardPresent success"); 
        #endif
        
        if ( !mfrc522.PICC_ReadCardSerial()) {
            #if DEBUG 
                Serial.println("Fail read card"); 
            #endif
            printLCD("Read card error", "", tm);
            delay(1000);
        } else {
            state = STATE_CARD_FOUND;;
        }
    }
}

void stateInitError(tmElements_t tm) 
{
    printLCD("Initialization", "Error", tm);
}

void stateFileError(tmElements_t tm) 
{  
    printLCD("Open file error", "Reinsert USB", tm);
    if(!key.init()) {
        state = STATE_USB_WAIT;
    }
}

void stateCardFound(tmElements_t tm) 
{  
    if (!file_ptr.open(file_name, O_CREAT | O_RDWR | O_APPEND) || !file_ptr.println(readRfidUid())) {
        #if DEBUG 
            Serial.println("open file fail"); 
        #endif
        state = STATE_FILE_ERROR;
    } else {
        file_ptr.close();
      
        printLCD("Read Success", "", tm); 
        buzzer();
        state = STATE_CARD_WAIT;
    }
}

void stateUsbLost() 
{
    #if DEBUG 
        Serial.println("USB Out"); 
    #endif
    state = STATE_USB_WAIT;
}

void stateUsbFound(tmElements_t tm) 
{
    printLCD("USB Inserted", "", tm);

    #if DEBUG 
        Serial.println("createFileName"); 
    #endif
    sprintf(file_name, FILENAME_FORMAT, tm.Day, tm.Month, tmYearToCalendar(tm.Year),tm.Hour, tm.Minute, tm.Second);

    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    
    state = STATE_CARD_WAIT;
    delay(1000);
}

void stateClockError(tmElements_t tm)
{
    #if DEBUG 
        Serial.println("stateClockError"); 
    #endif
    printLCD("Clock Error. Set", "time to continue", tm);
}

tmElements_t getTime()
{
    tmElements_t tm;
    if (RTC.read(tm)) {
        #if DEBUG 
            Serial.println("RTC.read success"); 
        #endif
    } else {
        state = STATE_CLOCK_ERROR;
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

    #if DEBUG 
        Serial.println(content + " Printed"); 
    #endif
  
    return content;
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

void printLCD(char* content1, char* content2, tmElements_t tm)
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print(content1);
    lcd.setCursor(0, 1);
    
    if(content2[0] == 0) {
        char buff[20];
        
        sprintf(buff, TIME_FORMAT, tm.Day, tm.Month,tm.Hour, tm.Minute, tm.Second);
        lcd.print(buff);
    } else {
        lcd.print(content2);
    }
}
