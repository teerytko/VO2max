//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Mini version
//
// TTGO T-Display: SDA-Pin21, SCL-Pin22
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*Board: ESP32 Dev Module
Upload Speed: 921600
CPU Frequency: 240Mhz (WiFi/BT)
Flash Frequency: 80Mhz
Flash Mode: QIO
Flash Size: 4MB (32Mb)
Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)
Core Debug Level: None
PSRAM: Disabled*/

// Set this to the correct printed case venturi diameter
#define DIAMETER 20

#define VERBOSE // additional debug logging

#include <Arduino.h>
#include "esp_adc_cal.h" // ADC calibration data
#include <EEPROM.h>      // include library to read and write settings from flash
#define ADC_EN 14        // ADC_EN is the ADC detection enable port
#define ADC_PIN 34
int vref = 1100;

#include <SPI.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <Wire.h>
#include "DFRobot_OxygenSensor.h" //Library for Oxygen sensor
#include "SCD30.h"                //Library for CO2 sensor

// declarations for bluetooth serial --------------
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

// declarations for BLE ---------------------
#include <BLE2902.h> // used for notifications 0x2902: Client Characteristic Configuration
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

const String Version = "V2.2 2023/01/23";

byte bpm;

byte heart[8] = {0b00001110, 60, 0, 0, 0, 0, 0, 0}; // defines the BT heartrate characteristic

// Byte[0]: flags: 0b00001110:
// not used/n.u./n.u./RR value available/Energy val.av./
// Sensor contact status/Sens.cont.supported/HR Format: (0: uint_8)
// Byte[1]: HR (uint_8)
// Byte[2]: Energy in J MSB
// Byte[3]: Energy in J LSB
// Byte[4]: RR
// Byte[5]: RR
// Byte[6]: ?
// Byte[7]: ?

byte hrmPos[1] = {2};

bool _BLEClientConnected = false;

// heart rate service
#define heartRateService BLEUUID((uint16_t)0x180D)
BLECharacteristic heartRateMeasurementCharacteristics(BLEUUID((uint16_t)0x2A37), BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic sensorPositionCharacteristic(BLEUUID((uint16_t)0x2A38), BLECharacteristic::PROPERTY_READ);
BLEDescriptor heartRateDescriptor(BLEUUID((uint16_t)0x2901));
BLEDescriptor sensorPositionDescriptor(BLEUUID((uint16_t)0x2901)); // 0x2901: Characteristic User Description

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer) { _BLEClientConnected = true; };

    void onDisconnect(BLEServer *pServer) { _BLEClientConnected = false; }
};

// ------------------------------------------

// Starts Screen for TTGO device
TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// Label of oxygen sensor
DFRobot_OxygenSensor Oxygen;
#define COLLECT_NUMBER 10           // collect number, the collection range is 1-100.
#define Oxygen_IICAddress ADDRESS_3 // I2C  label for o2 address

// Defines button state for adding wt
const int buttonPin1 = 0;
const int buttonPin2 = 35;
int wtTotal = 0;
int buttonPushCounter1 = 0; // counter for the duration of button1 pressed
int buttonState1 = 1;       // current state of the button
int buttonPushCounter2 = 0; // counter for the duration of button2 pressed
int buttonState2 = 1;       // current state of the button
int screenChanged = 0;
int screenNr = 1;
int HeaderStreamed = 0;
int HeaderStreamedBT = 0;
int DEMO = 0; // 1 = DEMO-mode

// ############################################
//  Select correct diameter depending on printed
//  case dimensions:
// ############################################

// Defines the size of the Venturi openings for the  calculations of AirFlow
float area_1 = 0.000531; // = 26mm diameter
#if (DIAMETER == 20)
float area_2 = 0.000314; // = 20mm diameter
#elif (DIAMETER == 19)
float area_2 = 0.000284; // = 19mm diameter
#else // default
float area_2 = 0.000201; // = 16mm diameter
#endif

float rho = 1.225;     // ATP conditions: density based on ambient conditions, dry air
float rhoSTPD = 1.292; // STPD conditions: density at 0°C, MSL, 1013.25 hPa, dry air
float rhoBTPS = 1.123; // BTPS conditions: density at ambient  pressure, 35°C, 95% humidity
float massFlow = 0;
float volFlow = 0;
float volumeTotal = 0;      // variable for holding total volume of breath
float pressure = 0.0;       // differential pressure of the venturi nozzle
float pressThreshold = 0.2; // threshold for starting calculation of VE
float volumeVE = 0.0;
float volumeVEmean = 0.0;
float volumeExp = 0.0;

// ######## Edit correction factor based on flow measurment with calibration syringe ############

// float correctionSensor = 1.0;   // correction factor

// ##############################################################################################

// Basic defaults in settings, saved to eeprom
struct
{
    int version = 1;              // Make sure saved data is right version
    float correctionSensor = 1.0; // calculated from 3L calibration syringe
    float weightkg = 75.0;        // Standard-body-weight
    bool co2_on = false;          // CO2 sensor active
    bool bmp_on = false;          // Pressure sensor sensor active
} settings;

float TimerVolCalc = 0.0;
float Timer5s = 0.0;
float Timer1min = 0.0;
float TimerVO2calc = 0.0;
float TimerVO2diff = 0.0; // used for integral of calories
float TimerStart = 0.0;
float TotalTime = 0.0;
String TotalTimeMin = String("00:00");
int readVE = 0;
float TimerVE = 0.0;
float DurationVE = 0.0;

float lastO2 = 0;
float initialO2 = 0;
float co2 = 0;
float calTotal = 0;
float vo2Cal = 0;
float vo2CalH = 0;        // calories per hour
float vo2CalDay = 0.0;    // calories per day
float vo2CalDayMax = 0.0; // highest value of calories per day
float vo2Max = 0;         // value of vo2Max/min/kg, calculated every 30 seconds
float vo2Total = 0.0;     // value of total vo2Max/min
float vo2MaxMax = 0;      // Best value of vo2 max for whole time machine is on

float respq = 0.0;      // respiratory quotient in mol VCO2 / mol VO2
float co2ppm = 0.0;     // CO2 sensor in ppm
float co2perc = 0.0;    // = CO2ppm /10000
float initialCO2 = 0.0; // initial value of CO2 in ppm
float vco2Total = 0.0;
float vco2Max = 0.0;
float co2temp = 0.0; // temperature CO2 sensor
float co2hum = 0.0;  // humidity CO2 sensor (not used in calculations)

float freqVE = 0.0;     // ventilation frequency
float freqVEmean = 0.0; // mean ventilation frequency

float expiratVol = 0.0; // last expiratory volume in L
float volumeTotalOld = 0.0;
float volumeTotal2 = 0.0;
float TempC = 15.0;    // Air temperature in Celsius barometric sensor BMP180
float PresPa = 101325; // uncorrected (absolute) barometric pressure

float Battery_Voltage = 0.0;

// Forward declarations
uint16_t readVoltage();     // read battery voltage
float readCO2();         // read CO2 sensor
float readO2();         // read CO2 sensor
void CheckInitialCO2(); // check initial CO2 value
void CheckInitialO2();  // check initial O2 value
void showScreen(float o2, float co2, float respq);      // show screen on OLED
void ReadButtons();     // read buttons
void tftScreen1(float o2, float co2, float respq);      // show screen 1 on TFT
void tftParameters();   // show parameters on TFT
void GetWeightkg();     // get weight from scale

void loadSettings()
{
    // Check version first.
    int version = EEPROM.read(0);
    if (version == settings.version)
    {
        for (int i = 0; i < sizeof(settings); ++i)
            ((byte *)&settings)[i] = EEPROM.read(i);
    }
}

void saveSettings()
{
    bool changed = false;
    for (int i = 0; i < sizeof(settings); ++i)
    {
        byte b = EEPROM.read(i);
        if (b != ((byte *)&settings)[i])
        {
            EEPROM.write(i, ((byte *)&settings)[i]);
            changed = true;
        }
    }
    if (changed)
        EEPROM.commit();
}

//----------------------------
void setup()
{
    EEPROM.begin(sizeof(settings));

    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);

    // defines ADC characteristics for battery voltage
    /*
      ADC_EN is the ADC detection enable port
      If the USB port is used for power supply, it is turned on by default.
      If it is powered by battery, it needs to be set to high level
    */
    // setup for analog digital converter used for battery voltage ---------
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    // Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    }
    else
    {
        // Serial.println("Default Vref: 1100mV");
    }

    // init display ----------
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    readVoltage();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("VO2max", 0, 25, 4);
    tft.drawString(Version, 0, 50, 4);
    tft.drawString("Initialising...", 0, 75, 4);
    // check for DEMO mode ---------
    if (!digitalRead(buttonPin2))
    { // DEMO Mode if button2 is pressed during power on
        DEMO = 1;
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("DEMO-MODE!", 0, 100, 4);
    }
    delay(3000);
    tft.fillScreen(TFT_BLACK);

    // init serial communication  ----------
    Wire.begin();
    Serial.begin(115200); // drop to 9600 to see if improves reliability
    if (!Serial)
    {
        tft.drawString("Serial ERROR!", 0, 0, 4);
    }
    else
    {
        tft.drawString("Serial ok", 0, 0, 4);
    }

    // init serial bluetooth -----------
    if (!SerialBT.begin("VO2max"))
    { // Start Bluetooth with device name
        tft.drawString("BT NOT ready!", 0, 25, 4);
    }
    else
    {
        tft.drawString("BT ready", 0, 25, 4);
    }

    // init O2 sensor DF-Robot -----------
    if (!Oxygen.begin(Oxygen_IICAddress))
    {
        tft.drawString("O2 ERROR!", 0, 75, 4);
    }
    else
    {
        tft.drawString("O2 ok", 0, 75, 4);
    }

    // init CO2 sensor Sensirion SCD30 -------------
    // check if sensor is connected?
    scd30.initialize();
    scd30.setAutoSelfCalibration(0);
    while (!scd30.isAvailable())
    {
        tft.drawString("CO2init..", 120, 75, 4);
    }
    tft.drawString("CO2 ok", 120, 75, 4);

    CheckInitialO2();
    // Disable CO2 for now
    // CheckInitialCO2();


    bpm = 30;  // initial test value

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    tft.drawCentreString("Ready...", 120, 55, 4);

    TimerVolCalc = millis(); // timer for the volume (VE) integral function
    Timer5s = millis();
    Timer1min = millis();
    TimerVO2calc = millis(); // timer between VO2max calculations
    TimerStart = millis();   // holds the millis at start
    TotalTime = 0;
    // BatteryBT(); // TEST for battery discharge log
    // ++++++++++++++++++++++++++++++++++++++++++++
}

//----------------------------------------------------------------------------------------------------------
//                  MAIN PROGRAM
//----------------------------------------------------------------------------------------------------------

void loop()
{
    TotalTime = millis() - TimerStart; // calculates actual total time

    // VO2max calculation, tft display and excel csv every 5s --------------
    if ((millis() - TimerVO2calc) > 5000 &&
        pressure < pressThreshold)
    { // calls vo2maxCalc() for calculation Vo2Max every 5 seconds.
        TimerVO2diff = millis() - TimerVO2calc;
        TimerVO2calc = millis(); // resets the timer
        float co2 = readCO2();
        float o2 = readO2();

        /*if (TotalTime >= 10000)*/ {
            showScreen(o2, co2, o2 / co2);
            readVoltage();
        }
        // send BLE data ----------------

        bpm = int(vo2Max + 0.5);
        heart[1] = (byte)bpm;

        int energyUsed = calTotal * 4.184; // conversion kcal into kJ
        heart[3] = energyUsed / 256;
        heart[2] = energyUsed - (heart[3] * 256);
        delay(100);

    }

    if (millis() - Timer1min > 30000)
    {
        Timer1min = millis(); // reset timer
                              // BatteryBT(); //TEST für battery discharge log ++++++++++++++++++++++++++++++++++++++++++
    }

    TimerVolCalc = millis(); // part of the integral function to keep calculation volume over time
    // Resets amount of time between calcs
}

//----------------------------------------------------------------------------------------------------------
//                  FUNCTIONS
//----------------------------------------------------------------------------------------------------------

void CheckInitialO2()
{
    // check initial O2 value -----------
    initialO2 = Oxygen.ReadOxygenData(COLLECT_NUMBER); // read and check initial VO2%
    if (initialO2 < 20.00)
    {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setCursor(5, 5, 4);
        tft.println("INITIAL O2% LOW!");
        tft.setCursor(5, 30, 4);
        tft.println("Wait to continue!");
        while (digitalRead(buttonPin1))
        {
            initialO2 = Oxygen.ReadOxygenData(COLLECT_NUMBER);
            tft.setCursor(5, 67, 4);
            tft.print("O2: ");
            tft.print(initialO2);
            tft.println(" % ");
            tft.setCursor(5, 105, 4);
            tft.println("Continue              >>>");
            delay(500);
        }
        if (initialO2 < 20.00)
            initialO2 = 20.90;
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(5, 5, 4);
        tft.println("Initial O2% set to:");
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(5, 55, 4);
        tft.print(initialO2);
        tft.println(" % ");
        delay(5000);
    }
}

//--------------------------------------------------

void CheckInitialCO2()
{ // check initial CO2 value
    readCO2();
    initialCO2 = co2ppm;

    if (initialCO2 > 1000)
    {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setCursor(5, 5, 4);
        tft.println("INITIAL CO2 HIGH!");
        tft.setCursor(5, 30, 4);
        tft.println("Wait to continue!");
        while (digitalRead(buttonPin1))
        {
            readCO2();
            initialCO2 = co2ppm;
            tft.setCursor(5, 67, 4);
            tft.print("CO2: ");
            tft.print(initialCO2, 0);
            tft.println(" ppm ");
            tft.setCursor(5, 105, 4);
            tft.println("Continue              >>>");
            delay(500);
        }
        if (initialCO2 > 1000)
            initialCO2 = 1000;
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(5, 5, 4);
        tft.println("Initial CO2 set to:");
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(5, 55, 4);
        tft.print(initialCO2, 0);
        tft.println(" ppm");
        delay(5000);
    }
}

//--------------------------------------------------

void ConvertTime(float ms)
{
    long inms = long(ms);
    int h, m, s;
    String strh, strm, strs;
    s = (inms / 1000) % 60;
    m = (inms / 60000) % 60;
    h = (inms / 3600000) % 24;
    strs = String(s);
    if (s < 10)
        strs = String("0") + strs;
    strm = String(m);
    if (m < 10)
        strm = String("0") + strm;
    strh = String(h);
    if (h < 10)
        strh = String("0") + strh;
    TotalTimeMin = String(strh) + String(":") + String(strm) + String(":") + String(strs);
}

//--------------------------------------------------
void BatteryBT()
{
    // HeaderStreamedBT = 1;// TEST: Deactivation of header
    if (HeaderStreamedBT == 0)
    {
        SerialBT.print("Time");
        SerialBT.print(",");
        SerialBT.println("Voltage");
        HeaderStreamedBT = 1;
    }
    SerialBT.print(float(TotalTime / 1000), 0);
    SerialBT.print(",");
    SerialBT.println(Battery_Voltage);
}

//--------------------------------------------------
float readO2()
{
    float oxygenData = Oxygen.ReadOxygenData(COLLECT_NUMBER);
    lastO2 = oxygenData;
    if (lastO2 > initialO2)
        initialO2 = lastO2; // correction for drift of O2 sensor

    if (DEMO == 1)
        lastO2 = initialO2 - 4; // TEST+++++++++++++++++++++++++++++++++++++++++++++
    return lastO2;
}

//--------------------------------------------------

float readCO2()
{
    float result[3] = {0};

    if (scd30.isAvailable())
    {
        scd30.getCarbonDioxideConcentration(result);

        co2ppm = result[0];
        if (co2ppm >= 40000)
        { // upper limit of CO2 sensor warning
            // tft.fillScreen(TFT_RED);
            tft.setTextColor(TFT_WHITE, TFT_RED);
            tft.drawCentreString("CO2 LIMIT!", 120, 55, 4);
        }

        if (DEMO == 1)
            co2ppm = 30000; // TEST+++++++++++++++++++++++++++++++++++++++++++++
        if (initialCO2 == 0)
            initialCO2 = co2ppm;
        co2perc = co2ppm / 10000;
        co2temp = result[1];
        co2hum = result[2];

        float co2percdiff = (co2ppm - initialCO2) / 10000; // calculates difference to initial CO2
        if (co2percdiff < 0)
            co2percdiff = 0;

        // VCO2 calculation is based on changes in CO2 concentration (difference to baseline)
        // vco2Total = volumeVEmean * rhoBTPS / rhoSTPD * co2percdiff * 10; // = vco2 in ml/min (* co2% * 10 for L in ml)
        // vco2Max = vco2Total / settings.weightkg;                         // correction for wt
        // respq = (vco2Total * 44) / (vo2Total * 32);                      // respiratory quotient based on molarity
        // CO2: 44g/mol, O2: 32 g/mol
        // if (isnan(respq))
        //     respq = 0; // correction for errors/div by 0
        //if (respq > 1.5)
        //    respq = 0;

#ifdef VERBOSE
        Serial.print("Carbon Dioxide Concentration is: ");
        Serial.print(result[0]);
        Serial.println(" ppm");
        Serial.print("Temperature = ");
        Serial.print(result[1]);
        Serial.println(" ℃");
        Serial.print("Humidity = ");
        Serial.print(result[2]);
        Serial.println(" %");
#endif
    }
    return result[0];
}

//--------------------------------------------------
void showScreen(float o2, float co2, float respq)
{ // select active screen
    ConvertTime(TotalTime);
    tft.setRotation(1);
    switch (screenNr)
    {
    case 1:
        tftScreen1(o2, co2, respq);
        break;
    case 6:
        tftParameters();
        break;
    default:
        // if nothing else matches, do the default
        // default is optional
        break;
    }
}

//--------------------------------------------------------
void tftScreen1(float o2, float co2, float respq)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(5, 5, 4);
    tft.print("Time  ");
    tft.setCursor(120, 5, 4);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println(TotalTimeMin);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    tft.setCursor(5, 30, 4);
    tft.print("O2 ");
    tft.setCursor(120, 30, 4);
    tft.println(o2);

    tft.setCursor(5, 80, 4);
    tft.print("VCO2 ");
    tft.setCursor(120, 80, 4);
    tft.println(co2);

    tft.setCursor(5, 105, 4);
    tft.print("RQ ");
    tft.setCursor(120, 105, 4);
    tft.println(respq);
}

//--------------------------------------------------------
void tftParameters()
{

    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);

    tft.setCursor(5, 5, 4);
    tft.print("*C");
    tft.setCursor(120, 5, 4);
    tft.println(co2temp, 1);

    tft.setCursor(5, 30, 4);
    tft.print("hPA");
    tft.setCursor(120, 30, 4);
    tft.println((PresPa / 100));

    tft.setCursor(5, 55, 4);
    tft.print("kg/m3");
    tft.setCursor(120, 55, 4);
    tft.println(rho, 4);

    tft.setCursor(5, 80, 4);
    tft.print("kg");
    tft.setCursor(45, 80, 4);
    tft.println(settings.weightkg, 1);

    tft.setCursor(120, 80, 4);
    tft.print("cor");
    tft.setCursor(180, 80, 4);
    tft.println(settings.correctionSensor, 2);

    tft.setCursor(5, 105, 4);
    tft.print("inO2%");
    tft.setCursor(120, 105, 4);
    tft.println(initialO2);
}

//--------------------------------------------------------
void ReadButtons()
{
    buttonState1 = digitalRead(buttonPin1);
    buttonState2 = digitalRead(buttonPin2);
    if (buttonState1 == LOW)
    {
        buttonPushCounter1++;
    }
    else
    {
        buttonPushCounter1 = 0;
    }
    if (buttonState2 == LOW)
    {
        buttonPushCounter2++;
    }
    else
    {
        buttonPushCounter2 = 0;
    }
}
//---------------------------------------------------------

void GetWeightkg()
{

    Timer5s = millis();
    int weightChanged = 0;
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.drawString("Enter weight in kg", 20, 10, 4);
    tft.drawString(String(settings.weightkg), 48, 48, 7);

    while ((millis() - Timer5s) < 5000)
    {
        ReadButtons();

        if (buttonPushCounter1 > 0)
        {
            settings.weightkg = settings.weightkg - 0.5;
            if (buttonPushCounter1 > 8)
                settings.weightkg = settings.weightkg - 1.5;
            weightChanged = 1;
        }

        if (buttonPushCounter2 > 0)
        {
            settings.weightkg = settings.weightkg + 0.5;
            if (buttonPushCounter2 > 8)
                settings.weightkg = settings.weightkg + 1.5;
            weightChanged = 1;
        }

        if (settings.weightkg < 20)
            settings.weightkg = 20;
        if (settings.weightkg > 200)
            settings.weightkg = 200;
        if (weightChanged > 0)
        {
            tft.fillScreen(TFT_BLUE);
            tft.drawString("New weight in kg is:", 10, 10, 4);
            tft.drawString(String(settings.weightkg), 48, 48, 7);
            weightChanged = 0;
            Timer5s = millis();
        }
        delay(200);
    }
}

//---------------------------------------------------------

uint16_t readVoltage()
{
    uint16_t v = analogRead(ADC_PIN);
    Battery_Voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    if (Battery_Voltage >= 4.3)
        tft.setTextColor(TFT_BLACK, TFT_WHITE); // USB powered, charging
    if (Battery_Voltage < 4.3)
        tft.setTextColor(TFT_BLACK, TFT_GREEN); // battery full
    if (Battery_Voltage < 3.9)
        tft.setTextColor(TFT_BLACK, TFT_YELLOW); // battery half
    if (Battery_Voltage < 3.7)
        tft.setTextColor(TFT_WHITE, TFT_RED); // battery critical
    tft.setCursor(0, 0, 4);
    tft.print(String(Battery_Voltage) + "V");
    return v;
}

//---------------------------------------------------------
