#include "vo2_ble_service.h"
    
void VO2BleServer::onConnect(BLEServer *pServer) {
    _BLEClientConnected = true; 
};
void VO2BleServer::onDisconnect(BLEServer *pServer) {
    _BLEClientConnected = false;
}

bool VO2BleServer::isClientConnected() const {
    return _BLEClientConnected;
}

bool VO2BleServer::initialize() {
    BLEDevice::init("VO2max Sensor");
    pBLEServer = BLEDevice::createServer();
    pBLEServer->setCallbacks(this);

    pBLEService = pBLEServer->createService(VO2MAX_SERVICE_UUID);
    vo2Characteristic = pBLEService->createCharacteristic(VO2_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    vco2Characteristic = pBLEService->createCharacteristic(VCO2_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    rqCharacteristic = pBLEService->createCharacteristic(RQ_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);

    // Add descriptors for each characteristic
    vo2Characteristic->addDescriptor(new BLE2902());
    vco2Characteristic->addDescriptor(new BLE2902());
    rqCharacteristic->addDescriptor(new BLE2902());

    pBLEService->start();
    pBLEServer->getAdvertising()->start();
    return true;
}

void VO2BleServer::pushVO2Data(float vo2Max) {
    if (vo2Characteristic) {
        vo2Characteristic->setValue(vo2Max);
        vo2Characteristic->notify();
    }
}

void VO2BleServer::pushVCO2Data(float vco2Max) {
    if (vco2Characteristic) {
        vco2Characteristic->setValue(vco2Max);
        vco2Characteristic->notify();
    }
}

void VO2BleServer::pushRQData(float respq) {
    if (rqCharacteristic) {
        rqCharacteristic->setValue(respq);
        rqCharacteristic->notify();
    }
}
