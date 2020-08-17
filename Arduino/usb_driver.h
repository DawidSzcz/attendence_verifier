#ifndef USBDriver_h
#define USBDriver_h

#include "Arduino.h"
#include <UsbFat.h>
#include <masstorage.h>

class USBDriver
{
  public:
    USBDriver(USB &usb, BulkOnly &bulk, UsbFat &key);
    bool init();
  private:
    bool _is_inserted;
    USB _usb;
    BulkOnly _bulk;
    UsbFat _key;
};


#endif USBDriver_h
