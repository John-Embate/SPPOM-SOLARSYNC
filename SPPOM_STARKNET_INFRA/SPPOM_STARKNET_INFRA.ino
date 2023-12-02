#include <Servo.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include "RTClib.h"
#include "DHT.h"
#include <SD.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PCF8574.h>
#include "Adafruit_VEML7700.h"
#include <Adafruit_INA219.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"

#define WIFI_SSID //++ "Enter your WiFi SSID" ++//
#define WIFI_PASSWORD //++ "Enter your WiFi Password" ++//
#define CS_pin D8 //memory card pin
#define LUX_IRRADIANCE_CONVERSION_FACTOR 0.0079
#define DELAY_TIME 5000
#define BLINKING_TIME 700
#define PCF_LED_ON LOW
#define PCF_LED_OFF HIGH

MPU9250_asukiaaa mpu9250_sensor(MPU9250_ADDRESS_AD0_HIGH);
RTC_DS3231 rtc;
DHT dht(D3, DHT22); //DHT dht_sensor(DHTPIN, DHTTYPE)
Servo servo1;
Servo servo2;
Adafruit_VEML7700 veml = Adafruit_VEML7700();
Adafruit_INA219 ina219;
PCF8574 pcf8574(0x20,D2,D1);

const char* SCOPE_ID = //++ Enter your Scope ID ++//;
const char* DEVICE_ID = //++ Enter your Device ID ++//;
const char* DEVICE_KEY = //++ Enter your Device Key ++//;


float humidity, temp, heat_index_celsius;
float mpu9250_roll , mpu9250_pitch;
float lux, irradiance;
float current_mA = 0, loadvoltage_V = 0;
unsigned long previousMillis = 0, LED_previousMillis = 0;
int servo1_angle=-45, servo2_angle = 0;                         // servo1_angle starts on -45 to correct logic error on first sweep
int year_, month_, day_, hour_, minute_, second_;               // Real-time clock data
bool start_servo_1=true, servo2_move=true;
bool ledState = PCF_LED_OFF;                                    // for led indicators timing of BLINKING_TIME
bool azure_indicator = false, rtc_indicator=false, accelerometer_indicator=false, dht22_indicator=false, SDcard_indicator=false, INA219_indicator=false, irradiance_indicator=false;
                                                                // for LED indicators (for easy debugging purposes and warning)
bool INA219_valid = false;                                      // use to check INA219 if its disconnected, and also for reconnecting if it was disconnected.
bool VEML7700_valid = false;                                    // used to check veml7700 if its disconnected at the beginning of program, and also for reconnecting if it was disconnected.

//function declarations
void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"
char *log_print(bool log_=true);
char *to_string(float *);

typedef struct{
  PCF8574 *pcf8574; // pointer to pcf8574 object
  uint8_t ledPin;
  bool *ledIndicator; //led_indicator attached to a PCF_Flasher led
}PCF_Flasher;

PCF_Flasher led1={&pcf8574, P0, &azure_indicator};
PCF_Flasher led2={&pcf8574, P1, &rtc_indicator};
PCF_Flasher led3={&pcf8574, P2, &accelerometer_indicator};
PCF_Flasher led4={&pcf8574, P3, &dht22_indicator};
PCF_Flasher led5={&pcf8574, P4, &SDcard_indicator};
PCF_Flasher led6={&pcf8574, P5, &INA219_indicator};
PCF_Flasher led7={&pcf8574, P6, &irradiance_indicator};

void setup() {

  //pinMode(CS_pin, OUTPUT); //memory card pin set (if Arduino-based)
  Serial.begin(115200);
  Serial.println(F("Starting Device..."));

  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  servo1.attach(D4);
  servo2.attach(D0);
  servo1.write(0);
  servo2.write(0);

  Wire.begin();
  mpu9250_sensor.setWire(&Wire);  //set address
  mpu9250_sensor.beginAccel();    //begin getting data from accelerometer

  rtc.begin();

  dht.begin();

  pcf8574.pinMode(P0,OUTPUT);
  pcf8574.pinMode(P1,OUTPUT);
  pcf8574.pinMode(P2,OUTPUT);
  pcf8574.pinMode(P3,OUTPUT);
  pcf8574.pinMode(P4,OUTPUT);
  pcf8574.pinMode(P5,OUTPUT);
  pcf8574.pinMode(P6,OUTPUT);
  pcf8574.pinMode(P7,OUTPUT);
  pcf8574.begin();

  if (ina219.begin()) {
    Serial.println("INA219 found!!!");    
    ina219.setCalibration_16V_400mA();
    INA219_valid = true;  
  }else{
    Serial.println("Failed to find INA219 chip");
  }

  if (veml.begin()) {
    Serial.println(F("Veml7700 sensor found"));
    VEML7700_valid = true;
  }else{
    Serial.println(F("Veml7700 sensor not found"));
  }
  
  //SD Card Initialization
  if (SD.begin(CS_pin)){
    Serial.println(F("SD card is ready to use."));
  }
  else {
    Serial.println(F("SD card initialization failed."));
  }

 if (context != NULL) {
    Serial.println(F("context not null"));
  }
}

void loop() {

  unsigned long currentMillis = millis();
  
  if(servo2_move){
    Serial.print(F("Moving Servo2 to angle: "));
    Serial.println(servo2_angle);
    servo2.write(servo2_angle);
    servo2_move=false;
  }

  if (start_servo_1) {
    servo1_angle += (servo1_angle < 180) ? 45 : -180; // if i = 180, servo2 will go back to beginning
    servo1.write(servo1_angle);
    start_servo_1 = false;
  }
  
  if (currentMillis - previousMillis >= DELAY_TIME) {
    char *message;
    previousMillis = currentMillis;             // update previousMillis to create non-blocking delay of _?_ seconds for saving & updating data, servo2.write(), and sending data to azure
    updatetime();                               // update variables --> year_, month_, day_, hour_, minute_, second_
    updateHumTempHIC();                         // update variables --> humidity, temp, heat_index_celsius
    updateIMUData();                            // update variables --> mpu9250_roll, mpu9250_pitch
    updateLuxirradiance();                      // update variables --> lux, irradiance
    updateVoltageCurrent();                     // update varaibles --> loadvoltage_V, current_mA
    message = log_print();                      // updated variables are printed in serial monitor if 'true' is passed as an argument to log_ parameter                             
    send_toazure(message);                      // send updated variables/new data to azure      
    SD_save();                                  // updated variables are saved in SD card
    servo2_angle += (servo2_angle < 92) ? 23 : -92;      // if servo2_angles = 180, servo2 will go back to beginning
    start_servo_1 = (servo2_angle == 0) ? true : false;    // sevo1 will start to move if servo2_angle goes back to 0
    servo2_move=true;                           // after 5 seconds servo2 can move to newly updated servo2_angle
  }

    if (currentMillis - LED_previousMillis >= BLINKING_TIME){
    LED_previousMillis = currentMillis;
    ledState=(ledState==PCF_LED_OFF)?PCF_LED_ON:PCF_LED_OFF;
    ledUpdate(&led1);
    ledUpdate(&led2);
    ledUpdate(&led3);
    ledUpdate(&led4);
    ledUpdate(&led5);
    ledUpdate(&led6);
    ledUpdate(&led7);
    if(azure_indicator + rtc_indicator + accelerometer_indicator + dht22_indicator + SDcard_indicator + INA219_indicator + irradiance_indicator < 7){
      pcf8574.digitalWrite(P7,PCF_LED_ON); // TurnOnRedLED if any problem exist with the sensors/commands
    }
    else{
      pcf8574.digitalWrite(P7,PCF_LED_OFF);
    }
  }

}

void ledUpdate(PCF_Flasher *led){
  if(*(led->ledIndicator)){
    led->pcf8574->digitalWrite(led->ledPin,ledState);
  }else{
    led->pcf8574->digitalWrite(led->ledPin,PCF_LED_OFF);
  }
}

void updatetime() {
  DateTime now = rtc.now();
  year_ = now.year();
  month_ = now.month();
  day_ = now.day();
  hour_ = now.hour();
  minute_ = now.minute();
  second_ = now.second();
  rtc_indicator = (year_ < 2022)?false:true; //rtcLED_indicator_off -> (if RTC module resets time back to 2000, low battery, disconnected etc.)
}

void updateIMUData(){

  if (mpu9250_sensor.accelUpdate() == 0) {
    float aX = mpu9250_sensor.accelX();
    float aY = mpu9250_sensor.accelY();
    float aZ = mpu9250_sensor.accelZ();
//    float aSqrt = mpu9250_sensor.accelSqrt(); //use only for finding relative yaw
    mpu9250_roll  = atan2(aX ,(sqrt((aY * aY) + (aZ * aZ)))) * 57.3;
    mpu9250_pitch = atan2(aY ,(sqrt((aX * aX) + (aZ * aZ)))) * 57.3;
    accelerometer_indicator = true;
  }
  else {
    Serial.println(F("Cannot read accel values"));
    mpu9250_roll  = NAN;
    mpu9250_pitch = NAN;
    accelerometer_indicator = false; //accelerometerLED_indicator_off -> (if unable to get new accel values, mp9250 disconnected, etc.)
  }
}

void updateHumTempHIC() {
  humidity = dht.readHumidity();
  temp = dht.readTemperature();
  heat_index_celsius = dht.computeHeatIndex(temp, humidity, false); // Compute heat index in Celsius (isFahreheit = false)
  dht22_indicator = ((isnan(humidity) && isnan(temp))||humidity>=99.9)?false:true; //dht22LED_indicator_off -> (if unable to connect with dht22/disconnected, high saturated air, etc.)
}

void updateLuxirradiance(){
  
  if(!VEML7700_valid || !irradiance_indicator){ //if VEML7700 disconnected at the beginning of running program
    if (veml.begin()) {
      veml.setGain(VEML7700_GAIN_1_8);           //gain used for calculating resolution
      veml.setIntegrationTime(VEML7700_IT_25MS); //integration time for calculating resolution
                                                 //VEML7700_GAIN_1_8 and VEML7700_IT_25MS as arguments for 120,796 max possible illumination value according to datasheet
      veml.interruptEnable(false);               //no interrupts since we will only gather data for Lux
      VEML7700_valid = true;
    }
    else{
      VEML7700_valid = false;
      irradiance = NAN;
    }
  }
  if(VEML7700_valid){
    lux = veml.readLux();
    irradiance = lux * LUX_IRRADIANCE_CONVERSION_FACTOR;
    irradiance_indicator = (lux==0 || irradiance >=7817527.5)?false:true;//irradianceLED_indicator_off -> (if VEML7700 disconnected, VEML7700 completely blocked from the sun, VEML7700 broken, etc.)
            //irradiance outputs 7817527.5 if VEML7700 is disconnected        
    (irradiance >= 7817527.5)?irradiance=NAN:0;
  }


}

void updateVoltageCurrent(){
  if(!INA219_valid){
    if(ina219.begin()){
      ina219.setCalibration_16V_400mA();
      INA219_valid = true;
    }
    else{
      INA219_valid = false;
      loadvoltage_V = NAN;
      current_mA = NAN;
    }
  }
  if(INA219_valid){
      loadvoltage_V = ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV()/1000); // get total load voltage in Volts
      current_mA = ina219.getCurrent_mA();
      INA219_indicator = true;
      if(loadvoltage_V == 0 || current_mA < 0){
        loadvoltage_V = 0;    // This is when load or solar panel is not attached or disconnected, ...
        current_mA = 0;          // INA219 returns negative current values, approximately: (-0.61mA) when load or power source is not connected --> https://forums.adafruit.com/viewtopic.php?f=19&t=82365
        INA219_indicator = false; //INA219LED_indicator_off -> (if solar panel disconnected, solar panel is blocked from the sun, load disconnected, solar panel no enough power delivered, etc.)
      }
      else if(loadvoltage_V > 0 && current_mA == 0){ //when INA219 is disconnected 
        loadvoltage_V = NAN;
        current_mA = NAN;
        INA219_indicator = false;
        INA219_valid = false;
      }
  }
}

void SD_save(){

  (!SDcard_indicator)?SD.begin(CS_pin):0; //if memory card removed and reinserted
  File sdcard_file;
  sdcard_file = SD.open("data.txt", FILE_WRITE);
  if (sdcard_file) {
    sdcard_file.print(year_);
    sdcard_file.print("/");
    sdcard_file.print(month_);
    sdcard_file.print("/");
    sdcard_file.print(day_);
    sdcard_file.print(" ");
    sdcard_file.print(hour_);
    sdcard_file.print(":");
    sdcard_file.print(minute_);
    sdcard_file.print(":");
    sdcard_file.print(second_);
    sdcard_file.print(",");
    sdcard_file.print(servo1_angle);
    sdcard_file.print(",");
    sdcard_file.print(servo2_angle);
    sdcard_file.print(",");
    sdcard_file.print(mpu9250_pitch,4);
    sdcard_file.print(",");
    sdcard_file.print(mpu9250_roll,4);
    sdcard_file.print(",");
    sdcard_file.print(humidity,4);
    sdcard_file.print(",");
    sdcard_file.print(temp,4);
    sdcard_file.print(",");
    sdcard_file.print(heat_index_celsius,4);
    sdcard_file.print(",");
    sdcard_file.print(irradiance,4);
    sdcard_file.print(",");
    sdcard_file.print(loadvoltage_V,4);
    sdcard_file.print(",");
    sdcard_file.println(current_mA,4);
    sdcard_file.close(); // close the file
    SDcard_indicator = true; 
  }
  else { // if the file didn't open, print an error:
    Serial.println(F("Error opening 'data.txt'..."));
    SDcard_indicator = false; //SDcard_indicator_off -> (if no memory card inserted, sdcard_module disconnected, CS_pin changed/disconnected, etc.)
  }
}

char *log_print(bool log_){
  
  static char msg[250];
  snprintf(msg, sizeof(msg), "{\"DateTime\": \"%d/%d/%d %d:%d:%d\", \"Servo1\": %d, \"Servo2\": %d, ", 
           year_, month_, day_, hour_, minute_, second_, servo1_angle, servo2_angle);          
  strcat(msg,"\"Pitch\": ");
  strcat(msg,to_string(&mpu9250_pitch));       //degrees
  strcat(msg,", \"Roll\": ");
  strcat(msg,to_string(&mpu9250_roll));       // degrees
  strcat(msg,", \"Humidity\": ");
  strcat(msg,to_string(&humidity));           // percent
  strcat(msg,", \"Temperature\": ");
  strcat(msg,to_string(&temp));               // degree Celsius
  strcat(msg,", \"HeatIndex\": ");
  strcat(msg,to_string(&heat_index_celsius)); // degree Celsius
  strcat(msg,", \"Irradiance\": ");
  strcat(msg,to_string(&irradiance));         // W/m2
  strcat(msg,", \"Voltage\": ");
  strcat(msg,to_string(&loadvoltage_V));      // V
  strcat(msg,", \"Current\": ");
  strcat(msg,to_string(&current_mA));         // mA
  strcat(msg,"}");

  if(log_){
      Serial.println(msg);
  }

  return msg;
}

char *to_string(float *num){
  static char float_val_holder[15];
  dtostrf(*num,5,4,float_val_holder);
  return float_val_holder;
}

void send_toazure(char *msg) {
  if (isConnected) {
      int pos = strlen(msg);
      int errorCode = iotc_send_telemetry(context, msg, pos);

      if (errorCode != 0) {
        LOG_ERROR("Sending message has failed with error code %d", errorCode);
      }

    iotc_do_work(context);  // do background work for iotc
    azure_indicator = true;
  } else {
    iotc_free_context(context);
    context = NULL;
    azure_indicator = false; //azureLED_indicator_off -> (if unable to connect with internet, unable to send data azure, etc.)
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }
}

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}
