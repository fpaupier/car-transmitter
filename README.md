# Car Transmitter

Remote control using ESPNOW to connect to RC car and control the car with an analog joystick.

Protocol used ESP NOW use a Githhub shield badge for the protocol used (ESP NOW) and another badge the type of board (ESP32) 

Related to other project for the car receievr

Note: this project is inspired from the  "Mecanum Wheel Robot Car & ESP-NOW Remote" from the drone Workshop
https://www.youtube.com/watch?v=dZttHOxIoek

## Controller illustration

Using an analog joystick to control the car the OLED display was a tentative to show the on board camera but black and
white OLED is not useful to display camera feed, to work on that  
![transmitter](./doc/controller-breadboard.jpg)

For the final version mounted on a perfboard see [here](./doc/controller-perfboard.png)

## Receiver pin illustration

![pin layout](./doc/pin-layout.png)

### Wiring diagram

| TTGO Pin | Joystick Pin | Description                                                                                          |
|----------|--------------|------------------------------------------------------------------------------------------------------|
| GND      | GND          | Ground connection                                                                                    |
| 3V3      | +5V          | Power supply (3.3V) despite the Joystick indicating 5V, otherwise you will read erratic analog value |
| GP32     | X AXIS       | Analog input for X-axis                                                                              |
| GP33     | Y AXIS       | Analog input for Y-axis                                                                              |
| GP27     | SWITCH       | Digital input for joystick button                                                                    |

## Getting the receiver MAC adress

ESPNOW protocol requires the MAC address of the receiver device (the other esp2 device on board of the RC car),
you can get it with this snippet to run on your receiver.

```cpp
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA); // Set as WiFi station
    delay(100); // Short delay for initialization
}

void loop() {
    Serial.print("ESP32 Receiver MAC Address: ");
    Serial.println(WiFi.macAddress());
}
```

## TFT-eSPI Library Modifications

The sketch we will be writing for the controller will use the TFT-eSPI Library by Bodmer. You can install this library
using the Library Manager in the Arduino IDE. You might have already installed it if you followed my instructions in the
article about using Round LCD Modules, but you may need to update it, as it needs to be at least version 2.4.79 to work
with the TTGO displays.

After installing it, you will need to modify a file in the library to work with the TTGO T-Display. Here is how you do
this:

1. Navigate to the TFT_eSPI folder in your libraries folder (which usually lives under your Arduino folder).
2. Look for User_Setup_Select.h and open it with a text editor.
3. Comment out line 30, which reads `#include <User_Setup.h>`
4. Uncomment line 61, which reads `#include <User_Setups/Setup25_TTGO_T_Display.h>`
5. Save the file.

Once you do this, the library will work with the TTGO module. Source: DroneBot
workshop https://dronebotworkshop.com/mecanum/

# Gotchas

- y Axis was inverted on my analog joystick so I had to adpt to this in my code
- 5V of the joystick is actually plugged to the 3.3V of the esp, otheriwse you have erratic measures
- to get the display working, amek sure to update the ``Setup25_TTGO_T_Display`` in the TFT-eSPI Library by Bodmer

## Bill of Materials

- TTGO T-Display - https://lilygo.cc/products/lilygo%C2%AE-ttgo-t-display-1-14-inch-lcd-esp32-control-board
- Analog joystick
- 3.7v battery or a power bank

Inspiration Mecanum Wheel Robot Car & ESP-NOW Remote from the drone Workshop
https://www.youtube.com/watch?v=dZttHOxIoek

[1]: #3v3
