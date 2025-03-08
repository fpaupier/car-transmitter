#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include "types.h"
#include "config.h"

class Communication {
public:
    Communication();

    bool begin();

    bool send(const struct_message &data);

    int getSignalStrength() const;

    bool isConnected() const;

private:
    esp_now_peer_info_t peerInfo;
    bool connected;
    int signalStrength;
    unsigned long lastSignalUpdate;
    unsigned long lastSendTime;

    static void onSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status);

    static Communication *instance;
};
