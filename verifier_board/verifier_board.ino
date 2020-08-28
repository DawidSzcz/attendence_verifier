#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <UsbFat.h>
#include <DS1307RTC.h>
#include <MemoryFree.h>

/*
 * Pin 13 SCK
 * Pin 12 MISO
 * Pin 11 MOSI
 */ 
#define SS_PIN 8
#define RST_PIN 9
#define BUZZER_PIN A1
#define CLOCK_PIN 10

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

const byte STATE_USB_WAIT = 1;
const byte STATE_CARD_WAIT = 2;
const byte STATE_CLOCK_ERROR = 3;
const byte STATE_INIT_ERROR = 4;
const byte STATE_USB_FOUND = 5;
const byte STATE_USB_LOST = 6;
const byte STATE_CARD_FOUND = 7;
const byte STATE_FILE_ERROR = 8;
const byte STATE_CLOCK_ERROR_USB_WAIT = 9;
const byte STATE_CLOCK_ERROR_DATE_FILE = 10;


const char TIME_FILE_NAME[] = "tfav";
const char FILENAME_FORMAT[] = "%02d-%02d-%02d-%02d_%02d_%02d";

const char LCD_INSERT[] PROGMEM = "Insert pendrive";
const char LCD_INSERT_1[] PROGMEM = "with time";
const char LCD_SCAN[] PROGMEM = "Scan card";
const char LCD_CARD_ERROR[] PROGMEM = "Read card error";
const char LCD_CLOCK_ERROR_1[] PROGMEM = "Clock Error. Set";
const char LCD_CLOCK_ERROR_2[] PROGMEM = "time to continue";
const char LCD_UPLOAD_FILE[] PROGMEM = "Upload time file";
const char LCD_INIT_ERROR_1[] PROGMEM = "Initialization";
const char LCD_INIT_ERROR_2[] PROGMEM = "Error";
const char LCD_REINSERT_1[] PROGMEM = "Open file error";
const char LCD_REINSERT_2[] PROGMEM = "Reinsert USB";
const char LCD_READ_SUCCESS[] PROGMEM = "Read Success";
const char LCD_INSERTED[] PROGMEM = "USB Inserted";
const char LCD_SET_SUCCESS[] PROGMEM = "Time set success";

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
    pinMode(CLOCK_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() 
{  
    Serial.print("freeMemory()=");
    Serial.println(freeMemory());
    tmElements_t tm;
    if (state != STATE_CLOCK_ERROR && state != STATE_CLOCK_ERROR_USB_WAIT && state != STATE_CLOCK_ERROR_DATE_FILE) {
         tm = getTime();
    }
    
    switch(state) {
        case STATE_USB_WAIT:
            stateUsbWait(tm);
            break;
        case STATE_CARD_WAIT:
            stateCardWait(tm);
            break;
        case STATE_CLOCK_ERROR:
            stateClockError();
            break;
        case STATE_CLOCK_ERROR_USB_WAIT:
            stateClockErrorUsbWait();
            break;
        case STATE_CLOCK_ERROR_DATE_FILE:
            stateClockErrorDateFile();
            break;
        case STATE_INIT_ERROR:
            stateInitError();
            break;
        case STATE_FILE_ERROR:
            stateFileError();
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
        Serial.println(F("stateUsb"));
    #endif
    printTmLCD(LCD_INSERT, tm);
    if (!initUSB(&usb)) {
        #if DEBUG 
            Serial.println(F("initUSB fail"));
        #endif
    } else if(!key.begin()) {
        #if DEBUG 
            Serial.println(F("key.begin fail"));
        #endif
        state = STATE_INIT_ERROR;
    } else {    
        #if DEBUG  
            Serial.println(F("USB Inserted"));
        #endif
        state = STATE_USB_FOUND;
    }
}

void stateCardWait(tmElements_t tm)
{
    printTmLCD(LCD_SCAN, tm);   
    #if DEBUG 
        Serial.println(F("stateCard"));
    #endif
    
    if (!key.init()) {
        state = STATE_USB_LOST;
    } else if (mfrc522.PICC_IsNewCardPresent()) {
        #if DEBUG 
            Serial.println(F("PICC_IsNewCardPresent success"));
        #endif
        
        if ( !mfrc522.PICC_ReadCardSerial()) {
            #if DEBUG 
                Serial.println(F("Fail read card"));
            #endif
            printTmLCD(LCD_CARD_ERROR, tm);
            delay(1000);
        } else {
            state = STATE_CARD_FOUND;;
        }
    }
}

void stateInitError() 
{
    printLCD(LCD_INIT_ERROR_1, LCD_INIT_ERROR_2);
}

void stateFileError() 
{  
    printLCD(LCD_REINSERT_1, LCD_REINSERT_2);
    if(!key.init()) {
        state = STATE_USB_WAIT;
    }
}

void stateCardFound(tmElements_t tm) 
{  
    if (!file_ptr.open(file_name, O_CREAT | O_RDWR | O_APPEND) || !file_ptr.println(readRfidUid())) {
        #if DEBUG 
            Serial.println(F("open file fail"));
        #endif
        state = STATE_FILE_ERROR;
    } else {
        file_ptr.close();
      
        printTmLCD(LCD_READ_SUCCESS, tm); 
        buzzer();
        state = STATE_CARD_WAIT;
    }
}

void stateUsbLost() 
{
    #if DEBUG 
        Serial.println(F("USB Out"));
    #endif
    state = STATE_USB_WAIT;
}

void stateUsbFound(tmElements_t tm) 
{
    printTmLCD(LCD_INSERTED, tm);

    #if DEBUG 
        Serial.println(F("createFileName"));
    #endif
    sprintf(file_name, FILENAME_FORMAT, tm.Day, tm.Month, tmYearToCalendar(tm.Year),tm.Hour, tm.Minute, tm.Second);

    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    
    state = STATE_CARD_WAIT;
    delay(1000);
}

void stateClockError()
{
    #if DEBUG 
        Serial.println(F("stateClockError"));
    #endif

    if(digitalRead(CLOCK_PIN)) {
        state = STATE_CLOCK_ERROR_DATE_FILE;
    } else {
        printLCD(LCD_CLOCK_ERROR_1, LCD_CLOCK_ERROR_2);
    }
}

void stateClockErrorUsbWait()
{
    printLCD(LCD_INSERT, LCD_INSERT_1);
    
    #if DEBUG 
        Serial.println(F("stateClockErrorInsertUsb"));
    #endif
    
    if (!initUSB(&usb)) {
        #if DEBUG 
            Serial.println(F("initUSB fail"));
        #endif
    } else if(!key.begin()) {
        #if DEBUG 
            Serial.println(F("key.begin fail"));
        #endif
        state = STATE_INIT_ERROR;
    } else {
        state = STATE_CLOCK_ERROR_DATE_FILE;
    }
}

void stateClockErrorDateFile()
{
    #if DEBUG 
        Serial.println(F("stateClockErrorDateFile"));
    #endif

    if(!key.init()) {
        state = STATE_CLOCK_ERROR_USB_WAIT;
    } else if(!key.exists(TIME_FILE_NAME)) {
        printLCD(LCD_UPLOAD_FILE, TIME_FILE_NAME);
    } else if (!file_ptr.open(TIME_FILE_NAME, O_READ)) {
      
    } else {
        char buff[20];
        int sec, min, hr, day, mon, year;
        tmElements_t tm;
        
        file_ptr.read(buff, 20);
        if(sscanf(buff, "%d/%d/%d %d:%d:%d", &day, &mon, &year, &hr, &min, &sec) != 6) {
          
        } else {
            tm.Day = day;
            tm.Month = mon;
            tm.Year = year;
            tm.Hour = hr;
            tm.Minute = min;
            tm.Second = sec;
            printTmLCD(LCD_SET_SUCCESS, tm);
            delay(1000);
            if (RTC.write(tm)) {
               state = STATE_CARD_WAIT;
            }            
        }
    }
}

tmElements_t getTime()
{
    tmElements_t tm;
    if (RTC.read(tm)) {
        #if DEBUG 
            Serial.println(F("RTC.read success"));
        #endif
    } else {
        state = STATE_CLOCK_ERROR;
    }

    return tm;
}

String readRfidUid()
{
    #if DEBUG 
        Serial.println(F("ReadRfidUid"));
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
        Serial.println(F("Buzzer"));
    #endif
    for(int i = 0; i < BUZZ_TIME; i += TONE_STEP) {
        noTone(BUZZER_PIN);
        tone(BUZZER_PIN, i, MAX_TONE * TONE_STEP / BUZZ_TIME);
        delay(TONE_STEP);
    }
}

void printTmLCD(char* content, tmElements_t tm)
{    
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    printProgmemLCD(content);
    lcd.setCursor(0, 1);
    printIntLcd(tm.Day); lcd.print("/");
    printIntLcd(tm.Month); lcd.print(" ");
    printIntLcd(tm.Hour); lcd.print(":");
    printIntLcd(tm.Minute); lcd.print(":");
    printIntLcd(tm.Second);
}

void printLCD(char* content1, char* content2)
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    printProgmemLCD(content1);
    lcd.setCursor(0, 1);
    printProgmemLCD(content2);
}

void printProgmemLCD(char* content)
{
    for (byte aux_byte = 0; aux_byte < strlen_P(content); aux_byte++) {
        lcd.print((char)pgm_read_byte_near(content + aux_byte));
    }
}

void printIntLcd(byte i) {
    if(i < 10) {
        lcd.print(0);
    }
    lcd.print(i);
}
