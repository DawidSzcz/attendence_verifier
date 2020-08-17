#include "Arduino.h"

#include <UsbFat.h>
#include <masstorage.h>
#include "usb_driver.h"

USBDriver::USBDriver(USB &usb, BulkOnly &bulk, UsbFat &key) :
  _usb(usb),
  _bulk(bulk),
  _key(key)
{
};

bool USBDriver::init()
{
  if (!initUSB(&_usb)) { // to robi coß takiego co wywala rfid
    Serial.println("initUSB failed");
    return;    
  } else {
    Serial.println("initUSB success");
  }
  if (!_key.begin()) {
    Serial.println(F("key.begin failed"));
    return;
  } else {
    Serial.println("key.begin success");
  }
};
