#include <Arduino.h>
#include <unity.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "Adafruit_BMP280.h" // Library for BMP280 ambient temp and pressure, set correct I2C address 0x76
#include "DFRobot_OxygenSensor.h" //Library for Oxygen sensor
#include "Omron_D6FPH.h" //Library for Oxygen sensor

DFRobot_OxygenSensor Oxygen;
Omron_D6FPH Pressure;
#define Oxygen_IICAddress ADDRESS_3 // I2C  label for o2 address
String STR_TO_TEST;

void setUp(void) {
    // set stuff up here
    STR_TO_TEST = "Hello, world!";
}

void tearDown(void) {
    // clean stuff up here
    STR_TO_TEST = "";
}

void test_string_concat(void) {
    String hello = "Hello, ";
    String world = "world!";
    TEST_ASSERT_EQUAL_STRING(STR_TO_TEST.c_str(), (hello + world).c_str());
}

void test_pressure_sensor_init(void) {
    bool init = Pressure.begin(MODEL_0025AD1);
    if (!init)
        Serial.print("Pressure sensor init error!\n");
    else
        Serial.print("Pressure sensor init ok");
    TEST_ASSERT_TRUE_MESSAGE(init, "Pressure sensor init error!");
    TEST_ASSERT_TRUE_MESSAGE(Pressure.isConnected(), "Pressure sensor is not connected");
}

void test_pressure_sensor_get_pressure(void) {
    bool init = Pressure.begin(MODEL_0025AD1);
    TEST_ASSERT_TRUE_MESSAGE(init, "Pressure sensor init error!");
    float pressure = Pressure.getPressure();
    TEST_ASSERT_NOT_EQUAL_MESSAGE(pressure, 0, "Pressure pressure read invalid!");   
    Serial.print("pressure data: ");
    Serial.print(pressure);
    Serial.print("\n");

}

void test_pressure_sensor_get_temp(void) {
    bool init = Pressure.begin(MODEL_0025AD1);
    TEST_ASSERT_TRUE_MESSAGE(init, "Pressure sensor init error!");
    float temp = Pressure.getTemperature();
    TEST_ASSERT_NOT_EQUAL_MESSAGE(temp, 0, "Pressure temp read invalid!");   
    Serial.print("temp data: ");
    Serial.print(temp);
    Serial.print("\n");

}

void test_oxygen_sensor_init(void) {
    bool init = Oxygen.begin(Oxygen_IICAddress);
    if (!init)
        Serial.print("O2 init error!\n");
    else
        Serial.print("O2 init ok");
    TEST_ASSERT_TRUE_MESSAGE(init, "O2 init error!");
}

void test_oxygen_sensor_read_data(void) {
    bool init = Oxygen.begin(Oxygen_IICAddress);
    TEST_ASSERT_TRUE_MESSAGE(init, "O2 init error!");
    for (int i = 0; i < 10; i++) {
        float data = Oxygen.ReadOxygenData(10);
        Serial.print("O2 data: ");
        Serial.print(data);
        Serial.print("\n");
        TEST_ASSERT_NOT_EQUAL_MESSAGE(data, 0, "O2 read error!");
    }
}

void setup()
{
    Serial.begin(115200); // drop to 9600 to see if improves reliability

    delay(2000); // service delay
    UNITY_BEGIN();

    RUN_TEST(test_string_concat);
    RUN_TEST(test_oxygen_sensor_init);
    RUN_TEST(test_oxygen_sensor_read_data);
    RUN_TEST(test_pressure_sensor_init);
    RUN_TEST(test_pressure_sensor_get_pressure);
    RUN_TEST(test_pressure_sensor_get_temp);

    UNITY_END(); // stop unit testing
}

void loop()
{
}