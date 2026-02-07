#pragma once

// declarations for bluetooth serial --------------
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// declarations for BLE ---------------------
#include <BLE2902.h> // used for notifications 0x2902: Client Characteristic Configuration
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Service UUID
const char VO2MAX_SERVICE_UUID[] = "12345678-1234-5678-1234-56789abcdef0";
// Separate Characteristic UUIDs
const char VO2_CHAR_UUID[] = "12345678-1234-5678-1234-56789abcdef1";
const char VCO2_CHAR_UUID[] = "12345678-1234-5678-1234-56789abcdef2";
const char RQ_CHAR_UUID[] = "12345678-1234-5678-1234-56789abcdef3";


class VO2BleServer : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
    bool isClientConnected() const;
    bool initialize();
    void pushVO2Data(float vo2Max);
    void pushVCO2Data(float vco2Max);
    void pushRQData(float respq);
private:
    bool _BLEClientConnected = false;
    // BLE server and JSON characteristic for telemetry
    BLEServer *pBLEServer;
    BLEService *pBLEService;
    BLECharacteristic *vo2Characteristic; // raw value of VO2 in ml/min
    BLECharacteristic *vco2Characteristic; // raw value of VCO2 in ml/min
    BLECharacteristic *rqCharacteristic; // raw value of RQ in mol VCO2 / mol VO2
    BluetoothSerial SerialBT;

};
