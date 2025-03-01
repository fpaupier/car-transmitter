# Car Transmitter

Remote control using ESPNOW to connect to RC car

## Controller illustration

Using an analog joystick to control the car the OLED display was a tentative to show the on board camera but black and white OLED is not useful to display camera feed, to work on that  
![transmitter](./doc/transmitter.jpg)

## Getting the receiver MAC adress

ESPNOW protocol requires the MAC address of the receiver device,
you can get it with this snippet

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

