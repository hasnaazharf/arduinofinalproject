#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <FirebaseArduino.h>
#include <ESP8266HTTPClient.h>
#include <RTClib.h>
#include <Wire.h>
#include "secret.h"

/* Predefine WiFi attribute */
   char ssid[] = SECRET_SSID; 
   char pass[] = SECRET_PASS;

/* Predefine Web Server attribute */  
   char host[] = SECRET_HOST;

/* Predefine Firebase Attribute */
    #define FIREBASE_HOST "hasna-smarthome.firebaseio.com"
    #define FIREBASE_AUTH "cLJ4s5FUhMb3mOYtzd2Hgh106Y6NNePcGxHLDshk"

WiFiClient  client;
RTC_DS3231 rtc;

/* Predefine timer */
   long previousRelayMillis = 0;
   long previousPIRMillis = 0;

/* Define PIN */
   int RelayOutput = D3;
   int PIRInput = D4;

/* Interval relay running */
   long interval = 600000; 

/* Predefine state */
   bool relayState = false;
   bool prevRelayState = false;
   bool motionState = false;
   bool prevMotionState = false;

/* Define Manual and Auto Variable */
  String autoMode = "";
  String manualLamp = ""; 
  String rtcHour, rtcMinute;

  int hourStart;                                                     
  int minStart;
  int hourEnd;                                                    
  int minEnd;

  String hourStartStr = "";
  String minStartStr = "";
  String hourEndStr = "";
  String minEndStr = "";
  

void setup() {

  Serial.begin(115200);
  delay(1000);

  /*Initialize Pin Arduino */
   pinMode(PIRInput,INPUT);
   pinMode(RelayOutput,OUTPUT);
   pinMode(LED_BUILTIN, OUTPUT);
   
  /* connect to wifi */
   WiFi.mode(WIFI_STA);
   connectWifi();

  /* Connect to Firebase */
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    
  /* Connect to RTC DS3231 */
    Wire.begin(D2, D1);
    rtc.begin();
}

void loop() {

  readMotion();
  delay(1000);

  autoMode = Firebase.getString("Mode/Auto_Mode");

    if (autoMode == "ON") {
      Serial.println("Automation Mode ON");
      auto_method();
    }

    else if (autoMode == "OFF") {
      Serial.println("Manual Mode ON");
      manual_method();
    }

  delay(1000);
}

/*=====================================================
 * 
 ======================================================*/

void readMotion(){
  /* 
   *  Calculate motion state
   *  - read PIR input
   *  - if PIR read motion, then immediately set motion state to true
   *  - if PIR doesn't read motion, wait for 6 secs (delay + detection block time)
   *  -- then if in 6 secs PIR still doesn't read motion, then set motion state to false
   */
   
  int motion = digitalRead(PIRInput);
  if (motion){
    updateMotionState(true);
  }else{
    unsigned long currentMillis = millis();
    if(currentMillis - previousPIRMillis > 6000) {
      previousPIRMillis = currentMillis;   
      updateMotionState(false);
    }
  }

  /*
   * Calculate Relay State
   * - if motion state is true, then immediately update relay state to true
   * - if motion state is false, wait for <interval> to turnoff relay
   */
  if (motionState){
    updateRelayState(true);
  }else{
    unsigned long currentMillis = millis();
    if(currentMillis - previousRelayMillis > interval) {
      previousRelayMillis = currentMillis;   
      updateRelayState(false);
    }
  }
}

/*
 * Update Motion State
 * - param: <bool> val
 * 
 * Update motion state
 * If state changed, then update to Web
 */
void updateMotionState(bool val){
  
  motionState = val;
  String motionPost = "{ \"value\":\"" + String(motionState) + "\" }";
  
  if (motionState != prevMotionState){
    //postHTTPSRequest(motionPost,"/api/log");
  }
  prevMotionState = motionState;
}

/*
 * Update Relay State
 * - param: <bool> val
 * 
 *  *if state is HIGH, we need to update relay then thinkspeak
 *  *if state is LOW, we need update thinkspeak then relay
 */
void updateRelayState(bool val){
  
  relayState = val;
  int watt = 9;
  String relayPost = "{ \"value\":" + String(relayState) + ", \"watt\":" + String(watt) + "}";
  
  if(val){ 
    digitalWrite(RelayOutput, HIGH); 
  }
  
  if (relayState != prevRelayState){
    postHTTPSRequest(relayPost,"/api/energy_daily");
  }
  
  if(!val){
    digitalWrite(RelayOutput, LOW); 
  }
  
  prevRelayState = relayState;
}

/*
 * Post HTTPS Request on Web Server
 */
void postHTTPSRequest(String postData, String url) {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  
  HTTPClient https;

  if (https.begin(*client, String(host) + String(url))) { 
    
    Serial.println("[HTTPS] Attempting to POST...");
    https.addHeader("Content-Type", "application/json");
    int httpCode = https.POST(postData);
    
    if (httpCode > 0) {
      Serial.printf("[HTTPS POST] Successful! Response Code: %d\n", httpCode); 
      }
       
    else {
      Serial.printf("[HTTPS POST] Failed!, Error: %s\n\r", https.errorToString(httpCode).c_str());
    }
    https.end();
    }
    
 else {
    Serial.printf("[HTTPS] Unable to connect\n\r");
 }
}

/*
 * Connect to WIFI
 */
void connectWifi(){
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }  
}

/*
 * Control Lamp Automatically
 */

void auto_method(){

  /* Get Current Time from RTC DS321*/
    DateTime now = rtc.now(); 
    rtcHour = now.hour();
    rtcMinute = now.minute();

    Serial.print("TIME = ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
  
    hourStart = Firebase.getInt("Mode/Auto_Time/hourStart");
    minStart = Firebase.getInt("Mode/Auto_Time/minuteStart");
    hourStartStr = String(hourStart);
    minStartStr = String(minStart);
    Serial.print("\nON = " + hourStartStr + ":" + minStartStr);

    hourEnd = Firebase.getInt("Mode/Auto_Time/hourEnd");
    minEnd = Firebase.getInt("Mode/Auto_Time/minuteEnd");
    hourEndStr = String(hourEnd);
    minEndStr = String(minEnd);
    Serial.println("\nOFF = " + hourEndStr + ":" + minEndStr);
 
  delay(1000);

  /*Compare Time from RTC DS321 with Firebase */
  
    if (rtcHour == hourStartStr && rtcMinute == minStartStr) {                                                          
      Serial.println("Lamp Turned ON");                         
      digitalWrite(LED_BUILTIN, LOW);                                                  
      digitalWrite(RelayOutput, HIGH);                                                         
    } 

    else if (rtcHour == hourEndStr && rtcMinute == minEndStr) {                                                  
      Serial.println("Lamp Turned OFF");
      digitalWrite(LED_BUILTIN, HIGH);                                               
      digitalWrite(RelayOutput, LOW);                                                         
    }

    else if (Firebase.failed()) { 
      Serial.print("Firebase Error"); 
      Serial.println(Firebase.error());   
      return; 
    }
}

/*
 * Control Lamp Manually
 */

void manual_method() {

  manualLamp = Firebase.getString("Mode/Manual_Lamp_Status");

  if (manualLamp == "ON") {                                                          
    Serial.println("Led Turned ON");                         
    digitalWrite(LED_BUILTIN, LOW);                                                  
    digitalWrite(RelayOutput, HIGH);                                                         
  } 
  else if (manualLamp == "OFF") {                                                  
    Serial.println("Led Turned OFF");
    digitalWrite(LED_BUILTIN, HIGH);                                               
    digitalWrite(RelayOutput, LOW);                                                         
  }
  else if (Firebase.failed()) { 
    Serial.print("setting /number failed:"); 
    Serial.println(Firebase.error());   
    return; 
  }
}
