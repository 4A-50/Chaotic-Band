//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//Capacitive Touch Libary
#include <CapacitiveSensor.h>

//Relay Pins
#define RelayPin 0

//680k resistor between pins 16 & 5, pin 5 is sensor pin
CapacitiveSensor cs1 = CapacitiveSensor(16,5);
//680k resistor between pins 16 & 4, pin 4 is sensor pin
CapacitiveSensor cs2 = CapacitiveSensor(16,4);
//680k resistor between pins 16 & 14, pin 14 is sensor pin
CapacitiveSensor cs3 = CapacitiveSensor(16,14);
//680k resistor between pins 16 & 12, pin 12 is sensor pin
CapacitiveSensor cs4 = CapacitiveSensor(16,12);
//680k resistor between pins 16 & 13, pin 13 is sensor pin
CapacitiveSensor cs5 = CapacitiveSensor(16,13);
//680k resistor between pins 16 & 15, pin 15 is sensor pin
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
int notes[6] = {32,
                33,
                34,
                35,
                36,
                37};

//The Amount Needed To Count As A 'Detection'
int dectAmount = 10000;

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

  //Relay Pin Setup
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);
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
  if(sensIn < dectAmount && playing[sensID] == false){
    SendMIDIMSG(notes[sensID], 127, 2, 0);
    playing[sensID] = true;
  }

  if(sensIn > dectAmount && playing[sensID] == true){
    SendMIDIMSG(notes[sensID], 0, 2, 0);
    playing[sensID] = false;
  }
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
