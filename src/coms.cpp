#include "coms.h"
#include "secrets.h" // Contains RECEIVER_MAC_ADDRESS

Communication *Communication::instance = nullptr;

Communication::Communication() :
        connected(false),
        signalStrength(0),
        lastSignalUpdate(0),
        lastSendTime(0) {
    instance = this;
}

bool Communication::begin() {
    // Initialize WiFi
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return false;
    }

    esp_now_register_send_cb(onSendCallback);

    // Register peer
    memcpy(peerInfo.peer_addr, RECEIVER_MAC_ADDRESS, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return false;
    }

    Serial.println("ESP-NOW communication initialized");
    return true;
}

void Communication::onSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (instance) {
        instance->connected = (status == ESP_NOW_SEND_SUCCESS);

        // Update signal strength based on success/failure
        if (millis() - instance->lastSignalUpdate > SIGNAL_UPDATE_INTERVAL) {
            if (instance->connected) {
                if (instance->signalStrength < MAX_SIGNAL_STRENGTH)
                    instance->signalStrength++;
            } else {
                if (instance->signalStrength > 0)
                    instance->signalStrength--;
            }
            instance->lastSignalUpdate = millis();
        }
    }
}

bool Communication::send(const struct_message &data) {
    if (millis() - lastSendTime < SEND_INTERVAL) {
        return true; // Not time to send yet
    }

    bool result = (esp_now_send(peerInfo.peer_addr, (uint8_t *) &data, sizeof(data)) == ESP_OK);
    if (!result) {
        Serial.println("Send Failed");
    }
    lastSendTime = millis();
    return result;
}

int Communication::getSignalStrength() const {
    return signalStrength;
}

bool Communication::isConnected() const {
    return connected;
}
