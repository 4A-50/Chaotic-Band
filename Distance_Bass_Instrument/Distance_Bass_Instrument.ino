//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//EEPROM Libary To Save Data To The EEPROM
#include <ESP_EEPROM.h>

//Relay Pins
#define RelayPin 0

//Ultrasonic Sensor Pins
const int trigPin = 12;
const int echoPin = 14;

//The Speed Of Sound
#define SOUND_VELOCITY 0.034

//Pin For The Calibrate Ultrasonic Distance Button
#define CalibratePin 16

//Can The Device Be Calibrated Bool
bool calibrateable = true;

//Last Read State Of The Cal Button
int lastReadState = HIGH;

//Max Length For The Ultrasonic Distance
int maxLength = 30;

//Fuzz Switch Pin
#define FuzzPin 5
//Arpergiator Switch Pin
#define ArpPin 4
//Arp Potentiometer Pin
#define PotPin A0
//BPM LED Pin
#define BPMLEDPin 13
//Divebomb Button Pin
#define DivebombPin 14

//Dry Sound MIDI Range
int dryStart = 115;
int dryEnd = 95;
int dryDivebomb = 94;

//Fuzz Sound MIDI Range
int fuzzStart = 93;
int fuzzEnd = 73;
int fuzzDivebomb = 72;

//The Last Recorded US Distance Reading
int lastDistance;
//The Last Sent MIDI Note Value
int lastMIDI;
//The Time The Last MIDI Note Was Sent
long lastSendTime;
//The Last Recorded BPM
int lastBPM;
//BPM LED State
int BPMLEDState = LOW;
//Last Time The BPM LED Changed State
long lastBPMLEDTime;
//Last Time The Divebomb Button Was Press
long lastDivebombTime;

//Master Mac Adress
static uint8_t MASTERMAC[]{0x42, 0x91, 0x51, 0x46, 0x34, 0xFD};

//Decodes The Incoming Message And Plays The Correct MIDI Info
void MessageDecoder(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Message Holder
  String message = "";

  //Loops Though All The Messages Data
  for (int i = 0; i < static_cast<int>(count); ++i) {
    message += static_cast<char>(buf[i]);
  }

  if(message == "Light_On"){
    digitalWrite(RelayPin, HIGH);
  }
  if(message == "Lights_Off"){
    digitalWrite(RelayPin, LOW);
  }
}

void setup() {
  Serial.begin(9600);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP("ESPNOW", nullptr, 3);
  WiFi.softAPdisconnect(false);

 bool ok = WifiEspNow.begin();
  if (!ok) {
    Serial.println("WifiEspNow.begin() failed");
    ESP.restart();
  }

  WifiEspNow.onReceive(MessageDecoder, nullptr);

  ok = WifiEspNow.addPeer(MASTERMAC);
  if (!ok) {
    Serial.println("WifiEspNow.addPeer() failed");
    ESP.restart();
  }

  //Start The EEPROM
  EEPROM.begin(16);

  //Check The EEPROM For Data
  if(EEPROM.percentUsed() > 0){
    EEPROM.get(0, maxLength);
  }

  lastDistance = maxLength;

  //Relay Pin Setup
  pinMode(RelayPin, OUTPUT);

  //Ultrasonic Pin Setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Calibate Pin Setup
  pinMode(CalibratePin, INPUT_PULLUP);

  //Switch Pin Setups
  pinMode(FuzzPin, INPUT_PULLUP);
  pinMode(ArpPin, INPUT_PULLUP);

  //BPM LED Pin Setup
  pinMode(BPMLEDPin, OUTPUT);

  //Divebomb Button Pin Setups
  pinMode(DivebombPin, INPUT_PULLUP);
}

void loop(){
  int currentDist = GetUFDistance();
  int currentMIDI = lastMIDI;
  int currentRawArpVal = analogRead(PotPin);
  int currentBPM = int(map(currentRawArpVal, 5, 1024, 150, 90));
  int currentArpPinState = digitalRead(ArpPin);

  //If The Divebomb Button Has Been Pressed
  if(digitalRead(DivebombPin) == LOW && lastDivebombTime + 2500 < millis()){
    //Checks If It's Playing The Dry Or Fuzz Signal
    if(digitalRead(FuzzPin) == LOW){
      SendMIDIMSG(fuzzDivebomb, 127, 1, 500);
    }else{
      SendMIDIMSG(dryDivebomb, 127, 1, 500);
    }

    //Sets The Last Divebomb Time To The Current Millis
    lastDivebombTime = millis();
  }

  //Flashes An LED At The Correct BPM Provided By The Pot (Via A Remap To A 90-150 BPM Range)
  if(lastBPMLEDTime + (60000 / currentBPM) < millis()){
    lastBPMLEDTime = millis();

    BPMLEDState = !BPMLEDState;

    digitalWrite(BPMLEDPin, BPMLEDState);

    //Checks The Arpeggator Is Active
    if(currentArpPinState == LOW && currentMIDI != -1 && lastDivebombTime + 2500 < millis()){
      SendMIDIMSG(currentMIDI, 127, 1, 500);
      lastSendTime = millis();
    }
  }

  //If The US Sensor Detects A Distance Change, And It's Been 1/4 Of A Second (To Stop Spam), And If It's Not Currently Divebombing
  if(currentDist != lastDistance && lastSendTime + 250 < millis() && lastDivebombTime + 2500 < millis()){
    //If The Current Distance Is Within The "Fret Board" Range
    if(currentDist <= maxLength){
      //Checks Weather It's Playing The Dry Or Fuzz Signal
      if(digitalRead(FuzzPin) == LOW){
        currentMIDI = int(map(currentDist, 2, maxLength, fuzzStart, fuzzEnd));
      }else{
        currentMIDI = int(map(currentDist, 2, maxLength, dryStart, dryEnd));
      }

      //Checks The Arpeggator Isn't Active
      if(currentArpPinState != LOW){
        //Sends The Message If It's A New Note Value (Not Just Distance)
        if(currentMIDI != lastMIDI){
          SendMIDIMSG(currentMIDI, 127, 1, 500);
          lastSendTime = millis();
        }
      }
    }else{
      if(currentArpPinState == LOW && currentMIDI != -1){
        currentMIDI = -1;
      }
    }
  }
  
  //If The Ultrasonic Sensor Can Be Distance Calibrated
  if(calibrateable == true){
    int calPinRead = digitalRead(CalibratePin);

    //Checks If The Calibrate Button Is Pressed
    if(calPinRead == LOW && lastReadState == HIGH){
      //Flash RELAY Lights

      //Gets The Max Distance
      maxLength = GetUFDistance();

      //Adds The Max Length To The EEPROM
      EEPROM.put(0, maxLength);
      boolean ok1 = EEPROM.commit();
      Serial.println((ok1) ? "First commit OK" : "Commit failed");

      //Allows It To Be Calibrated Once Per Boot
      calibrateable = false;
    }

    lastReadState = calPinRead;
  }

  lastDistance = currentDist;
  lastMIDI = currentMIDI;
  lastBPM = currentBPM;
}

//Sends A Message To The Master
void SendMIDIMSG(int note, int velocity, int mChannel, int playTime){
  //Creates A Buffer With A Size Of 60
  char msg[60];
  //Writes The Info To Buffer Whilst Getting It's Size
  int len = snprintf(msg, sizeof(msg), "N%iV%iC%iT%i", note, velocity, mChannel, playTime);

  //Sends The Message Via The WIFIESPNOW Libary
  WifiEspNow.send(MASTERMAC, reinterpret_cast<const uint8_t*>(msg), len);
}

//Works Out The Distance The US Sensor Can See
int GetUFDistance(){
  //Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  //Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  //Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  
  //Calculate the distance
  float distance = duration * SOUND_VELOCITY/2;

  return distance;
}
