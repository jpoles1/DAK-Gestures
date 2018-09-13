#include <string>
using namespace std;

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include "ESP8266WiFi.h"
#include <esp8266httpclient.h>

//Private Config
#include "config.h"

// Pins
#define APDS9960_SDA D2
#define APDS9960_SCL D3
#define APDS9960_INT D4
#define ERROR_LED_PIN D6

// Global Variables
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int isr_flag = 0;

void interruptRoutine() {
    isr_flag = 1;
}

void httpRequest(String apiURL){
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(apiURL);
        Serial.println("GET - " + apiURL);
        int httpCode = http.GET();
        Serial.println("Response Code - " + httpCode);
        if(httpCode != 200){
            digitalWrite(ERROR_LED_PIN, HIGH);
            String payload = http.getString();
            Serial.println("GET Error: " + payload);
        }
        else {
            digitalWrite(ERROR_LED_PIN, LOW);
        }
    }
    else {
        Serial.println("Disconnected, cannot send request!");
    }
}

void deviceCommandRequest(String device, String fname, String val){
    String reqURL = String(SERVER_URL) + "?key=" + SERVER_KEY + "&device=" + device + "&fname=" + fname + "&val=" + val;
    httpRequest(reqURL);
}

void wifiCheck(){
    if(WiFi.status() != WL_CONNECTED){
        Serial.print("Connecting to WiFi...");
    }
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    digitalWrite(ERROR_LED_PIN, LOW);
}

void sendPowerCommands(String powerSetting) {
    deviceCommandRequest("1", "power", powerSetting);
    deviceCommandRequest("2", "power", powerSetting);
    deviceCommandRequest("3", "power", powerSetting);
    deviceCommandRequest("4", "power", powerSetting);
    deviceCommandRequest("lightswitch", "power", powerSetting);
}

void handleGesture() {
    if ( apds.isGestureAvailable() ) {
        switch ( apds.readGesture() ) {
        case DIR_UP:
            Serial.println("UP");
            break;
        case DIR_DOWN:
            Serial.println("DOWN");
            break;
        case DIR_LEFT:
            Serial.println("LEFT");
            sendPowerCommands("on");
            break;
        case DIR_RIGHT:
            Serial.println("RIGHT");
            sendPowerCommands("off");
            break;
        case DIR_NEAR:
            Serial.println("NEAR");
            break;
        case DIR_FAR:
            Serial.println("FAR");
            break;
        default:
            Serial.println("NONE");
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, HIGH);

    //Setup I2C
    Wire.begin(APDS9960_SDA, APDS9960_SCL);

    // Set interrupt pin as input
    pinMode(APDS9960_INT, INPUT);

    // Initialize interrupt service routine
    attachInterrupt(APDS9960_INT, interruptRoutine, FALLING);

    // Initialize APDS-9960 (configure I2C and initial values)
    if ( apds.init() ) {
        Serial.println(F("APDS-9960 initialization complete"));
    } else {
        Serial.println(F("Something went wrong during APDS-9960 init!"));
    }

    // Start running the APDS-9960 gesture sensor engine
    if ( apds.enableGestureSensor(true) ) {
        Serial.println(F("Gesture sensor is now running"));
    } else {
        Serial.println(F("Something went wrong during gesture sensor init!"));
    }
    WiFi.begin(AP_SSID, AP_PASSWORD);
    wifiCheck();
}

void loop() {
    if( isr_flag == 1 ) {
        detachInterrupt(APDS9960_INT);
        handleGesture();
        isr_flag = 0;
        attachInterrupt(APDS9960_INT, interruptRoutine, FALLING);
    }
}