#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void BTsetup()
{
  SerialBT.begin("ESP32"); //Bluetooth device name
}
