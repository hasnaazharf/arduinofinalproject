#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

#include <RTClib.h>
#include <Wire.h>
#include "secret.h"

/*Define Wi-Fi and Firebase Attributes */
  char ssid[] = SECRET_SSID; 
  char pass[] = SECRET_PASS;
  #define FIREBASE_HOST "hasna-smarthome.firebaseio.com"
  #define FIREBASE_AUTH "cLJ4s5FUhMb3mOYtzd2Hgh106Y6NNePcGxHLDshk"
 
RTC_DS3231 rtc;
WiFiClient  client;

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
  
/* Define PIN */
  int lamp = D3;

void setup() {
  
  Serial.begin(115200);
  delay(1000);
  
  /*Initialize Pin Arduino */
    pinMode(LED_BUILTIN, OUTPUT);      
    pinMode(lamp, OUTPUT); 
                  
  /* Connect to WiFi */
    WiFi.mode(WIFI_STA);
    connectWifi();

  /* Connect to Firebase */
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  /* Connect to RTC DS3231 */
    Wire.begin(D2, D1);
    rtc.begin();
  }

void loop() {

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
      digitalWrite(lamp, HIGH);                                                         
    } 

    else if (rtcHour == hourEndStr && rtcMinute == minEndStr) {                                                  
      Serial.println("Lamp Turned OFF");
      digitalWrite(LED_BUILTIN, HIGH);                                               
      digitalWrite(lamp, LOW);                                                         
    }

    else if (Firebase.failed()) { 
      Serial.print("Firebase Error"); 
      Serial.println(Firebase.error());   
      return; 
    }
}

void manual_method() {

  manualLamp = Firebase.getString("Mode/Manual_Lamp_Status");

  if (manualLamp == "ON") {                                                          
    Serial.println("Led Turned ON");                         
    digitalWrite(LED_BUILTIN, LOW);                                                  
    digitalWrite(lamp, HIGH);                                                         
  } 
  else if (manualLamp == "OFF") {                                                  
    Serial.println("Led Turned OFF");
    digitalWrite(LED_BUILTIN, HIGH);                                               
    digitalWrite(lamp, LOW);                                                         
  }
  else if (Firebase.failed()) { 
    Serial.print("setting /number failed:"); 
    Serial.println(Firebase.error());   
    return; 
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
