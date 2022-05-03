//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//Relay Pins
#define RelayPin 0

//Master Mac Adress
static uint8_t MASTERMAC[]{0x42, 0x91, 0x51, 0x46, 0x34, 0xFD};

//PIR PIN
int pirPin = 5;

//Time Since Last Motion Detected
long lastMotionTime = 0;

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
  //Sets Up The Onboard LED & PIR Pin
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pirPin, INPUT);
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
}

void loop(){
  //Checks If Motion Has Been Detected And That It Has Been Enough Time Since The Last Reading
  if (digitalRead(pirPin) == HIGH && millis() - lastMotionTime > 10000) {
    //Turns The LED On
    digitalWrite(LED_BUILTIN, LOW); //For Some Reason On The NODEMCU LOW IS Actually On And HIGH Is OFF

    //Prints To Serial And Sends A MSG To Master
    Serial.println("Motion Detected");
    SendMIDIMSG(500, 127, 3, 2000);

    //Sets The Current Time To The Last Time
    lastMotionTime = millis();

    //Waits Then Turns The LED Off
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

//Sends A Message To The Master That It's Sensed Movement
void SendMIDIMSG(int note, int velocity, int mChannel, int playTime){
  //Creates A Buffer With A Size Of 60
  char msg[60];
  //Writes The Info To Buffer Whilst Getting It's Size
  int len = snprintf(msg, sizeof(msg), "N%iV%iC%iT%i", note, velocity, mChannel, playTime);

  //Sends The Message Via The WIFIESPNOW Libary
  WifiEspNow.send(MASTERMAC, reinterpret_cast<const uint8_t*>(msg), len);
}
