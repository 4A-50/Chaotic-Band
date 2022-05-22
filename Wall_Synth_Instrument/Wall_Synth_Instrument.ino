//WIFI ESP NOW Library
#include <WifiEspNow.h>

//Picks The Right WIFI Library For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//Capacitive Touch Library
#include <CapacitiveSensor.h>

//The Async Web Librarys
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//EEPROM Libary To Save Data To The EEPROM
#include <ESP_EEPROM.h>

//Relay Pins
#define RelayPin 0

//330k resistor between pins 16 & 5, pin 5 is sensor pin
CapacitiveSensor cs1 = CapacitiveSensor(16,5);
//330k resistor between pins 16 & 4, pin 4 is sensor pin
CapacitiveSensor cs2 = CapacitiveSensor(16,4);
//330k resistor between pins 16 & 14, pin 14 is sensor pin
CapacitiveSensor cs3 = CapacitiveSensor(16,14);
//330k resistor between pins 16 & 12, pin 12 is sensor pin
CapacitiveSensor cs4 = CapacitiveSensor(16,12);
//330k resistor between pins 16 & 13, pin 13 is sensor pin
CapacitiveSensor cs5 = CapacitiveSensor(16,13);
//330k resistor between pins 16 & 15, pin 15 is sensor pin
CapacitiveSensor cs6 = CapacitiveSensor(16,15);

//Master Mac Adress
static uint8_t MASTERMAC[]{0x42, 0x91, 0x51, 0x46, 0x34, 0xFD};

//If A Note Is Being Played
bool playing[6] = {false,
                   false,
                   false,
                   false,
                   false,
                   false};
                   
//The Notes To Play
int notes[6] = {60,
                50,
                41,
                38,
                33,
                28};

//The Amount Needed To Count As A 'Detection'
int dectAmount[6] = {3000,
                     5000,
                     4000,
                     4000,
                     5000,
                     3000};

//The Last Recorded Reading (Used For The Web Calibator)
int lastReading[6] = {0,
                      0,
                      0,
                      0,
                      0,
                      0};

//Web Server Object
AsyncWebServer server(80);

//Decodes The Incoming Message And Plays The Correct MIDI Info
void MessageDecoder(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Message Holder
  String message = "";

  //Loops Though All The Messages Data
  for(int i = 0; i < static_cast<int>(count); ++i) {
    message += static_cast<char>(buf[i]);
  }

  if(message == "Light_On"){
    digitalWrite(RelayPin, LOW);
  }
  if(message == "Lights_Off"){
    digitalWrite(RelayPin, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP("WallSynth", "WallSynth", 3);

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

  //Starts The EEPROM With A Size OF 24 Bytes (6 * 4(Int))
  EEPROM.begin(24);

  //Check The EEPROM For Data
  if(EEPROM.percentUsed() > 0){
    //Fills The Thresholds With The Saved Data
    for(int i = 0; i < 6; i++) {
      EEPROM.get(i * 4, dectAmount[i]);
    }
  }

  //Sets Up The Index Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  //Sets Up The Page To Pull Pad Data
  server.on("/padreadings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getPadValues().c_str());
  });

  //Sets Up The Page To Recive New Pad Thresholds
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    for(int i = 0; i < 6; i++) {
      if(request->hasParam("padIn" + String(i))){
        String getVal = request->getParam("padIn" + String(i))->value();
        dectAmount[i] = getVal.toInt();

        EEPROM.put(i * 4, dectAmount[i]);
      }
    }

    boolean ok1 = EEPROM.commit();
    Serial.println((ok1) ? "First commit OK" : "Commit failed");
    
    request->send(200, "text/plain", "OK");
  });


  // Start server
  server.begin();

  //Relay Pin Setup
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, HIGH);
  
  cs1.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs2.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs3.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs4.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs5.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs6.set_CS_AutocaL_Millis(0xFFFFFFFF);
}

void loop(){
  //Checks If The Pads Should Be Playing A Note
  CheckPlaying(cs1.capacitiveSensor(30), 0);
  CheckPlaying(cs2.capacitiveSensor(30), 1);
  CheckPlaying(cs3.capacitiveSensor(30), 2);
  CheckPlaying(cs4.capacitiveSensor(30), 3);
  CheckPlaying(cs5.capacitiveSensor(30), 4);
  CheckPlaying(cs6.capacitiveSensor(30), 5);

  delay(250);
}

//Checks If The Provided Senesor Should Be Playing Due To Its Capactance
void CheckPlaying(long sensIn, int sensID){
  Serial.print(sensIn);
  Serial.print("  |  ");
  Serial.println(sensID + 1);

  lastReading[sensID] = sensIn;

  //For Some Reason The 6th Pad (Array ID 5) Is Inverted So It's Value Is High Until Touched Where It Dips
  if(sensID == 5){
    if(sensIn < dectAmount[sensID] && playing[sensID] == false){
      Serial.println("Starting MIDI From Pad" + String(sensID + 1));
      SendMIDIMSG(notes[sensID], 127, 2, 0);
      playing[sensID] = true;
    }
  
    if(sensIn > dectAmount[sensID] && playing[sensID] == true){
      Serial.println("Ending MIDI From Pad" + String(sensID + 1));
      SendMIDIMSG(notes[sensID], 0, 2, 0);
      playing[sensID] = false;
    }
  }else{
    if(sensIn > dectAmount[sensID] && playing[sensID] == false){
      Serial.println("Starting MIDI From Pad" + String(sensID + 1));
      SendMIDIMSG(notes[sensID], 127, 2, 0);
      playing[sensID] = true;
    }
  
    if(sensIn < dectAmount[sensID] && playing[sensID] == true){
      Serial.println("Ending MIDI From Pad" + String(sensID + 1));
      SendMIDIMSG(notes[sensID], 0, 2, 0);
      playing[sensID] = false;
    }
  }
}

//Turns The Last Read Values To A String
String getPadValues(){
  return String(lastReading[0]) + " " + String(lastReading[1]) + " " + String(lastReading[2]) + " " + String(lastReading[3]) + " " + String(lastReading[4]) + " " + String(lastReading[5]);
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
