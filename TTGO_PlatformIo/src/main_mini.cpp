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
#define DIAMETER 18

#undef VERBOSE // additional debug logging

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
#include "Omron_D6FPH.h"          //Library for differential pressure sensor
#include "vo2_ble_service.h"          //Library for differential pressure sensor

// Starts Screen for TTGO device
TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

// Labels the pressure sensor
Omron_D6FPH presSensor;

// Label of oxygen sensor
DFRobot_OxygenSensor Oxygen;
#define COLLECT_NUMBER 10           // collect number, the collection range is 1-100.
#define Oxygen_IICAddress ADDRESS_3 // I2C  label for o2 address
const String Version = "V2.3 2026/02/07";

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
#elif (DIAMETER == 18)
float area_2 = 0.000254; // = 18mm diameter
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
float consumed_o2 = 0;
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
// if ble
VO2BleServer bleServer;

enum ventilationStates
{
    WAITING_PRESSURE,
    INSPIRATION,
    EXPIRATION,
    EXPIRATION_DONE
};
int ventilationState = WAITING_PRESSURE;
// Forward declarations
uint16_t readVoltage();     // read battery voltage
float readCO2();         // read CO2 sensor
float readO2();         // read CO2 sensor
float volumeCalc();         // (
void vo2maxCalc();
void CheckInitialCO2(); // check initial CO2 value
void CheckInitialO2();  // check initial O2 value
void showScreen(float o2, float co2, float respq, float vol);      // show screen on OLED
void ReadButtons();     // read buttons
void tftScreen1(float o2, float co2, float respq, float vol);      // show screen 1 on TFT
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
    
    // 
    if (bleServer.initialize())
    {
        tft.drawString("BLE init ok", 0, 25, 4);
    }
    else
    {
        tft.drawString("BLE init ERROR!", 0, 25, 4);
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
    CheckInitialCO2();
    // init flow/pressure sensor Omron D6F-PF0025AD1 (or D6F-PF0025AD2) ----------
    while (!presSensor.begin(MODEL_0025AMD2))
    {
        // Serial.println("Flow sensor error!");
        tft.drawString("Flow-Sensor ERROR!", 0, 100, 4);
    }
    // Serial.println("Flow-Sensor I2c connect success!");
    tft.drawString("Flow-Sensor ok", 0, 100, 4);
    delay(2000);

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
    float vol = volumeCalc();
    // VO2max calculation, tft display and excel csv every 5s --------------
    if (ventilationState == INSPIRATION) {
        float o2 = readO2();
        float co2 = readCO2();
        //showScreen(o2, co2, respq, vol);
        delay(100);
    }
    if (//(millis() - TimerVO2calc) > 5000 && pressure < pressThreshold) 
        ventilationState == EXPIRATION_DONE
    )
    { // calls vo2maxCalc() for calculation Vo2Max every 5 seconds.
        ventilationState = INSPIRATION;
        TimerVO2diff = millis() - TimerVO2calc;
        TimerVO2calc = millis(); // resets the timer
        float co2 = readCO2();
        float o2 = readO2();
        Serial.print("{ \"vco2\": {");
        Serial.print("\"vco2Total\": ");
        Serial.print(vco2Total);
        Serial.print(", \"vco2Max\": ");
        Serial.print(vco2Max);
        Serial.print(", \"respq\": ");
        Serial.print(respq);
        Serial.println("}}");

        vo2maxCalc();
        /*if (TotalTime >= 10000)*/
        {
            showScreen(o2, co2, respq, vol);
            readVoltage();
        }
        // send BLE data ----------------
        // Publish JSON telemetry via BLE (if a client connected)
        if (bleServer.isClientConnected())
        {
            bleServer.pushVO2Data(vo2Max);
            bleServer.pushVCO2Data(vco2Max);
            bleServer.pushRQData(respq);
        }
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

String ConvertTime(float ms)
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
    return TotalTimeMin;
}

//--------------------------------------------------
float readO2()
{
    float oxygenData = Oxygen.ReadOxygenData(COLLECT_NUMBER);
    lastO2 = oxygenData;
    if (lastO2 > initialO2)
        initialO2 = lastO2; // correction for drift of O2 sensor

    if (DEMO == 1)
        lastO2 = initialO2 - 4;
#ifdef VERBOSE
        Serial.print("O2: ");
        Serial.print(lastO2);
        Serial.print("\n");
#endif
    
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
        vco2Total = volumeVEmean * rhoBTPS / rhoSTPD * co2percdiff * 10; // = vco2 in ml/min (* co2% * 10 for L in ml)
        vco2Max = vco2Total / settings.weightkg;                         // correction for wt
        respq = (vco2Total * 44) / (vo2Total * 32);                      // respiratory quotient based on molarity
        // CO2: 44g/mol, O2: 32 g/mol
        if (isnan(respq))
             respq = 0; // correction for errors/div by 0
        if (respq > 1.5)
            respq = 0;

#ifdef VERBOSE
        Serial.print("Initial CO2: ");
        Serial.print(initialCO2);
        Serial.print("CO2: ");
        Serial.print(result[0]);
        Serial.print(" ppm ");
        // Serial.print("Temperature = ");
        Serial.print(result[1]);
        Serial.print(" ℃");
        // Serial.print("Humidity = ");
        Serial.print(result[2]);
        Serial.println(" %");
#endif
    }
    return result[0];
}

float volumeCalc()
{
#ifdef VERBOSE
    //Serial.print("TeemuR: VolumeCalc\n");
#endif
    // Read pressure from Omron D6F PH0025AD1 (or D6F PH0025AD2)
    float pressureraw = presSensor.getPressure();
    pressure = pressure / 2 + pressureraw / 2;

#if 0
    Serial.print("\nTeemuR: pressure: ");
    Serial.print(pressure);
    Serial.print("\n");
#endif

    if (isnan(pressure))
    { // isnan = is not a number,  unvalid sensor data
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.drawCentreString("VENTURI ERROR!", 120, 55, 4);
    }
    if (pressure > 266)
    { // upper limit of flow sensor warning
        // tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.drawCentreString("SENSOR LIMIT!", 120, 55, 4);
    }
    if (pressure < 0)
        pressure = 0;


    if (pressure < pressThreshold && readVE == 1)
    {
        if (ventilationState == EXPIRATION)
        {
            Serial.print("{ \"event\": \"EXPIRATION DONE\", \"time\": ");
            Serial.print("\""+ConvertTime(TotalTime)+"\"");
            Serial.println("}");

            ventilationState = EXPIRATION_DONE;
        }
        // read volumeVE
        readVE = 0;
        DurationVE = millis() - TimerVE;
        TimerVE = millis(); // start timerVE
        volumeExp = volumeTotal;
        volumeTotal = 0; // resets volume for next breath
        volumeVE = volumeExp / DurationVE * 60;
        volumeExp = volumeExp / 1000;
        volumeVEmean = (volumeVEmean * 3 / 4) + (volumeVE / 4); // running mean of one minute volume (VE)
        if (volumeVEmean < 1)
            volumeVEmean = 0;
        freqVE = 60000 / DurationVE;
        if (volumeVE < 0.1)
            freqVE = 0;
        freqVEmean = (freqVEmean * 3 / 4) + (freqVE / 4);
        if (freqVEmean < 1)
            freqVEmean = 0;

#if 1
        Serial.print("{ \"volume\": {");
        Serial.print("\"volumeExp\": ");
        Serial.print(volumeExp);
        Serial.print(", \"VE\": ");
        Serial.print(volumeVE);
        Serial.print(", \"VEmean\": ");
        Serial.print(volumeVEmean);
        Serial.print(", \"freqVE\": ");
        Serial.print(freqVE, 1);
        Serial.print(", \"freqVEmean\": ");
        Serial.print(freqVEmean, 1);
        Serial.println("}}");
        
#endif
    }
    //if (millis() - TimerVE > 5000)
    //    readVE = 1; // readVE at least every 5s

    if (pressure >= pressThreshold)
    { // ongoing integral of volumeTotal
#if 0
    Serial.print("\nTeemuR EXPIRATION: volumeTotal: ");
    Serial.print(volumeTotal);
    Serial.print("\n");
#endif
        ventilationState = EXPIRATION;

        if (volumeTotal > 50)
            readVE = 1;
//        Serial.print("TeemuR: rho = ");
//        Serial.print(rho);
//        Serial.println("\n");

        massFlow = 1000 * sqrt((abs(pressure) * 2 * rho) / ((1 / (pow(area_2, 2))) - (1 / (pow(area_1, 2))))); // Bernoulli equation
        volFlow = massFlow / rho;                                                                              // volumetric flow of air
        volFlow = volFlow * settings.correctionSensor;                                                         // correction of sensor calculations
        volumeTotal = volFlow * (millis() - TimerVolCalc) + volumeTotal;
        volumeTotal2 = volFlow * (millis() - TimerVolCalc) + volumeTotal2;
    }
    else if ((volumeTotal2 - volumeTotalOld) > 200)
    { // calculate actual expiratory volume
        expiratVol = (volumeTotal2 - volumeTotalOld) / 1000;
        volumeTotalOld = volumeTotal2;
    }
    return expiratVol;
}

void AirDensity()
{
    //co2temp = result[1];
    //co2hum = result[2];

    TempC = co2temp; // bmp.readTemperature(); // Temp from baro sensor BM280
    //Serial.print("TeemuR: AirDensity TempC = ");
    //Serial.print(TempC);
    //Serial.println("\n");

    // co2temp is temperature from CO2 sensor
    // PresPa = bmp.readPressure();
    //Serial.print("TeemuR: AirDensity co2temp = ");
    //Serial.print(co2temp);
    //Serial.println("\n");
    rho = PresPa / (co2temp + 273.15) / 287.058; // calculation of air density
    rhoBTPS = PresPa / (35 + 273.15) / 292.9;    // density at BTPS: 35°C, 95% humidity

    //Serial.print("TeemuR: AirDensity rho = ");
    //Serial.print(rho);
    //Serial.println("\n");
}

void vo2maxCalc()
{
    // V02max calculation every 5s
    AirDensity(); // calculates air density

#ifdef VERBOSE
    // Debug. compare co2
    Serial.print("\ninitialO2 ");
    Serial.print(initialO2);
    Serial.print("\nlastO2 ");
    Serial.print(lastO2);
    Serial.print("\nsens co2 ");
    Serial.println(co2perc);
#endif

    consumed_o2 = initialO2 - lastO2; // calculated level of consumed O2 based on Oxygen level loss
    if (consumed_o2 < 0)
        consumed_o2 = 0; // correction for sensor drift

    float vo2TotalIn = volumeVEmean * rhoBTPS / rhoSTPD * initialO2 / 100; // = vo2 in ml/min (* consumed_o2% * 10 for L in ml)
    float vo2TotalOut = volumeVEmean * rhoBTPS / rhoSTPD * lastO2 / 100; // = vo2 in ml/min (* consumed_o2% * 10 for L in ml)
    vo2Total = volumeVEmean * rhoBTPS / rhoSTPD * consumed_o2 * 1; // = vo2 in ml/min (* consumed_o2% * 10 for L in ml)
    vo2Max = vo2Total / settings.weightkg;                  // correction for wt
    if (vo2Max > vo2MaxMax)
        vo2MaxMax = vo2Max;

    vo2Cal = vo2Total / 1000 * 4.86;                     // vo2Max liters/min * 4.86 Kcal/liter = kcal/min
    calTotal = calTotal + vo2Cal * TimerVO2diff / 60000; // integral function of calories
    vo2CalH = vo2Cal * 60.0;                             // actual calories/min. * 60 min. = cal./hour
    vo2CalDay = vo2Cal * 1440.0;                         // actual calories/min. * 1440 min. = cal./day
    if (vo2CalDay > vo2CalDayMax)
        vo2CalDayMax = vo2CalDay;

    Serial.print("{ \"vo2\": {");
    Serial.print("\"vo2Total\": ");
    Serial.print(vo2Total);
    Serial.print(", \"consumed_o2\": ");
    Serial.print(consumed_o2);
    Serial.print(", \"vo2TotalIn\": ");
    Serial.print(vo2TotalIn);
    Serial.print(", \"vo2TotalOut\": ");
    Serial.print(vo2TotalOut);
    Serial.println("}}");
 
}

//--------------------------------------------------
void showScreen(float o2, float co2, float respq, float vol)
{
    // select active screen
    ConvertTime(TotalTime);
    tft.setRotation(1);
    switch (screenNr)
    {
    case 1:
        tftScreen1(o2, co2, respq, vol);
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
void tftScreen1(float o2, float co2, float respq, float vol)
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

    tft.setCursor(5, 55, 4);
    tft.print("VCO2 ");
    tft.setCursor(120, 55, 4);
    tft.println(co2);

    tft.setCursor(5, 80, 4);
    tft.print("RQ ");
    tft.setCursor(120, 80, 4);
    tft.println(respq);

    tft.setCursor(5, 105, 4);
    tft.print("Vol ");
    tft.setCursor(120, 105, 4);
    tft.println(vol, 3);
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
