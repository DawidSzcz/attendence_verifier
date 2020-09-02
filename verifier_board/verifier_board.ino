#include <SPI.h>
#include <LiquidCrystal.h>
#include <UsbFat.h>
#include <DS1307RTC.h>
#include <PN532_HSU.h>
#include <PN532.h>

/*
 * Pin 13 SCK
 * Pin 12 MISO
 * Pin 11 MOSI
 */ 
#define BUZZER_PIN A1
#define CLOCK_PIN A0

#define LCD_RESET_PIN 6
#define LCD_ENABLE_PIN 7
#define LCD_D4_PIN 2
#define LCD_D5_PIN 3
#define LCD_D6_PIN 4
#define LCD_D7_PIN 5

#define BUZZ_TIME 1000
#define MAX_TONE 2000
#define TONE_STEP 25
#define DEBUG false

const byte STATE_USB_WAIT = 1;
const byte STATE_USB_INIT = 2;
const byte STATE_CARD_WAIT = 3;
const byte STATE_CARD_FOUND = 4;
const byte STATE_INIT_ERROR = 5;
const byte STATE_USB_REMOVE = 6;
const byte STATE_USB_LOST = 7;
const byte STATE_FILE_ERROR = 8;
const byte STATE_CLOCK_ERROR = 9;
const byte STATE_CLOCK_ERROR_USB_WAIT = 10;
const byte STATE_CLOCK_ERROR_CLICK = 11;
const byte STATE_CLOCK_ERROR_RESET_FAIL = 12;

const char TIME_FILE_NAME[] = "tfav";

const char LCD_INSERT[] PROGMEM = "Insert pendrive";
const char LCD_INSERT_1[] PROGMEM = "with time";
const char LCD_SCAN[] PROGMEM = "Scan card";
const char LCD_CARD_ERROR[] PROGMEM = "Read error";
const char LCD_CLOCK_ERROR[] PROGMEM = "Clock Error";
const char LCD_UPLOAD_FILE[] PROGMEM = "Upload time";
const char LCD_INIT_ERROR[] PROGMEM = "Init Error";
const char LCD_FILE_ERROR[] PROGMEM = "Open file error";
const char LCD_REINSERT[] PROGMEM = "Reinsert USB";
const char LCD_READ_SUCCESS[] PROGMEM = "Read Success";
const char LCD_INSERTED[] PROGMEM = "USB Inserted";
const char LCD_REMOVED[] PROGMEM = "USB removed";
const char LCD_SET_SUCCESS[] PROGMEM = "Time set";
const char LCD_CLOCK_RESET_FAIL[] PROGMEM = "Clock Reset Fail";
const char LCD_CLICK_TO_RESET[] PROGMEM = "Click to reset";
const char LCD_WRONG_DATE_FORMAT[] PROGMEM = "Bad date format";
const char LCD_EMPTY[] PROGMEM = "";
const char TIME_FORMAT[] PROGMEM = "%d/%d/%d %d:%d:%d";
const char FILENAME_FORMAT[] PROGMEM = "%02d-%02d-%02d-%02d_%02d_%02d";
char file_name[20];

int state = STATE_USB_WAIT;

#define LOOP_DELAY 100

USB usb;
BulkOnly bulk(&usb);
UsbFat key(&bulk);
bool usb_inserted = false;
LiquidCrystal lcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN); // initialize the lcd
tmElements_t tm;
PN532_HSU pn532hsu(Serial);
PN532 nfc(pn532hsu);

void setup() 
{
    Serial.begin(9600);   // Initiate a serial communication 
    while (!Serial); 
    #if DEBUG
        Serial.println("SETUP");
    #endif

    nfc.begin();
    nfc.SAMConfig();

    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uid_length;

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(CLOCK_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() 
{  
    if (state == STATE_USB_WAIT || state == STATE_CARD_WAIT || state == STATE_CARD_FOUND || state == STATE_USB_INIT || state == STATE_USB_REMOVE) {
         setTime();
    }
    
    switch(state) {
        case STATE_USB_WAIT:
            stateUsbWait();
            break;
        case STATE_USB_INIT:
            stateUsbInit();
            break;
        case STATE_USB_REMOVE:
            stateUsbRemove();
            break;
        case STATE_CARD_WAIT:
            stateCardWait();
            break;
        case STATE_CLOCK_ERROR:
            stateClockError();
            break;
        case STATE_CLOCK_ERROR_USB_WAIT:
            stateClockErrorUsbWait();
            break;
        case STATE_CLOCK_ERROR_CLICK:
            stateClockErrorClick();
            break;
        case STATE_INIT_ERROR:
            stateInitError();
            break;
        case STATE_FILE_ERROR:
            stateFileError();
            break;
        case STATE_CARD_FOUND:
            stateCardFound();
            break;
        case STATE_USB_LOST:
            stateUsbLost();
            break;
        case STATE_CLOCK_ERROR_RESET_FAIL:
            stateClockErrorResetFail();
            break;
            
    }
    delay(LOOP_DELAY);
}

void stateUsbWait()
{
    printTmLCD(LCD_INSERT, tm);
    #if DEBUG 
        Serial.println(F("stateUsb")); 
    #endif
    
    if (!initUSB(&usb)) {
        #if DEBUG 
            Serial.println(F("initUSB fail")); 
        #endif
    } else {    
        #if DEBUG  
            Serial.println(F("USB Inserted")); 
        #endif
        state = STATE_USB_INIT;
    }
}

void stateCardWait()
{
    printTmLCD(LCD_SCAN, tm);   
    #if DEBUG 
        Serial.println(F("stateCard")); 
    #endif
    
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uid_length;

    #if DEBUG 
        Serial.println(F("key.init")); 
    #endif
    
    if (!key.init()) {
        state = STATE_USB_LOST;
    } else if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_length)) {
        #if DEBUG 
            Serial.println(F("readPassiveTargetID success")); 
        #endif
        state = STATE_CARD_FOUND;
    }
}

void stateInitError() 
{
    printLCD(LCD_INIT_ERROR, LCD_EMPTY);
    #if DEBUG 
        Serial.println(F("Init Error")); 
    #endif
    state = STATE_USB_REMOVE;;
    delay(1000);
}

void stateFileError() 
{  
    printLCD(LCD_FILE_ERROR, LCD_EMPTY);
    #if DEBUG 
        Serial.println(F("File Error")); 
    #endif

    state = STATE_USB_REMOVE;
    delay(1000);
}

void stateCardFound() 
{  
    #if DEBUG 
        Serial.println(F("stateCardFound")); 
    #endif
    File file_ptr;
    if (!file_ptr.open(file_name, O_CREAT | O_RDWR | O_APPEND | O_SYNC) || !file_ptr.println(readRfidUid())) {
        #if DEBUG 
            Serial.println(F("open file fail")); 
        #endif
        state = STATE_FILE_ERROR;
    } else {      
        printTmLCD(LCD_READ_SUCCESS, tm); 
        buzzer(false);
        file_ptr.close();
        state = STATE_CARD_WAIT;
    }
}

void stateUsbLost() 
{
    printLCD(LCD_REMOVED, LCD_EMPTY);
    #if DEBUG 
        Serial.println(F("USB Out")); 
    #endif
    state = STATE_USB_WAIT;
    buzzer(true);
}

void stateUsbInit() 
{
    printTmLCD(LCD_INSERTED, tm);
    #if DEBUG 
        Serial.println(F("createFileName")); 
    #endif

    if(!key.begin()) {
        #if DEBUG 
            Serial.println(F("key.begin fail")); 
        #endif
        state = STATE_INIT_ERROR;
    } else {
        char file_format[30];
        readProgmemIntoBuff(FILENAME_FORMAT, file_format);
        #if DEBUG 
            Serial.println(file_format); 
        #endif
        sprintf(file_name, file_format, tm.Day, tm.Month, tmYearToCalendar(tm.Year),tm.Hour, tm.Minute, tm.Second);
        state = STATE_CARD_WAIT;
    }
    delay(1000);
}

void stateUsbRemove()
{
    printTmLCD(LCD_REINSERT, tm);
    #if DEBUG 
        Serial.println(F("createFileName")); 
    #endif
        
    if(!key.init()) {
        state = STATE_USB_LOST;
    }
}

void stateClockError()
{
    printLCD(LCD_CLOCK_ERROR, LCD_EMPTY);
    #if DEBUG 
        Serial.println(F("stateClockError")); 
    #endif

    state = STATE_CLOCK_ERROR_USB_WAIT;
    delay(1000);
}

void stateClockErrorUsbWait()
{
    #if DEBUG 
        Serial.println(F("stateClockErrorInsertUsb")); 
    #endif

    File file_ptr;
    
    if (!initUSB(&usb)) {
        #if DEBUG 
            Serial.println(F("initUSB fail")); 
        #endif
        printLCD(LCD_INSERT, LCD_INSERT_1);
    } else if(!key.begin()) {
        #if DEBUG 
            Serial.println(F("key.begin fail")); 
        #endif
        state = STATE_INIT_ERROR;
    } else if(!key.exists(TIME_FILE_NAME)) {
        printLCD(LCD_UPLOAD_FILE, TIME_FILE_NAME);
    } else if (!file_ptr.open(TIME_FILE_NAME, O_READ)) {
        state = STATE_INIT_ERROR;
    } else {
        char buff[20];
        int sec, min, hr, day, mon, year;

        file_ptr.read(buff, 20);
        file_ptr.close();
        
        char time_buff[18];
        readProgmemIntoBuff(TIME_FORMAT, time_buff);
        
        if(sscanf(buff, time_buff, &day, &mon, &year, &hr, &min, &sec) != 6) {
            printLCD(LCD_WRONG_DATE_FORMAT, LCD_EMPTY);
        } else {
            tm.Day = day;
            tm.Month = mon;
            tm.Year = CalendarYrToTm(year);
            tm.Hour = hr;
            tm.Minute = min;
            tm.Second = sec;
            state = STATE_CLOCK_ERROR_CLICK;
        }
    }
}

void stateClockErrorClick()
{
    #if DEBUG 
        Serial.println(F("stateClockErrorDateFile")); 
    #endif

    if(1 != digitalRead(CLOCK_PIN)) {
        printLCD(LCD_CLICK_TO_RESET, LCD_EMPTY);
    } else if (!RTC.write(tm)) {
        state = STATE_CLOCK_ERROR_RESET_FAIL;
    } else {
        printTmLCD(LCD_SET_SUCCESS, tm);
        delay(1000);
        state = STATE_USB_INIT;
    }
}

void stateClockErrorResetFail()
{
    printLCD(LCD_CLOCK_RESET_FAIL, LCD_EMPTY);
}

void setTime()
{
    if (RTC.read(tm)) {
        #if DEBUG 
            Serial.println(F("RTC.read success")); 
        #endif
    } else {
        state = STATE_CLOCK_ERROR;
    }
}

void buzzer(bool reverse)
{
    #if DEBUG 
        Serial.println(F("Buzzer")); 
    #endif
    for(int i = 0; i < BUZZ_TIME; i += TONE_STEP) {
        noTone(BUZZER_PIN);
        tone(BUZZER_PIN, reverse ? MAX_TONE - (i * MAX_TONE / BUZZ_TIME) : i * MAX_TONE / BUZZ_TIME , TONE_STEP);
        delay(TONE_STEP);
    }
}

String readRfidUid()
{
    #if DEBUG
        Serial.println(F("ReadRfidUid"));
    #endif
    String content = "";
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uid_length;
    nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_length);

    
    content.concat(String(uid[0] < 0x10 ? "0" : ""));
    content.concat(String(uid[0], HEX));
    
    for (byte i = 1; i < uid_length; i++)
    {
         content.concat(String(uid[i] < 0x10 ? " 0" : " "));
         content.concat(String(uid[i], HEX));
    }

    #if DEBUG
        Serial.println(content + " Printed");
    #endif

    return content;
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

void readProgmemIntoBuff(char* content, char* buff)
{
    for (byte aux_byte = 0; aux_byte < strlen_P(content); aux_byte++) {
        buff[aux_byte] = (char)pgm_read_byte_near(content + aux_byte);
    }
    buff[strlen_P(content)] = '\0';
}

char* printIntLcd(byte i) {
    if(i < 10) {
        lcd.print(0);
    }
    lcd.print(i);
}
