//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//The MIDI Libary
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

//The PEER Class
#include <Peer.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//The Instrument Mac Addresses
//Should Be An Array OF Peer's But IDK Why That Isn't Working So This Will Have To Do For Now
static uint8_t PEER1[]{0x42, 0x91, 0x51, 0x51, 0x70, 0x5F};

//Decodes The Incoming Message And Plays The Correct MIDI Info
void MessageDecoder(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Var Holders To Write The Buffer Data Too
  bool noteBool = false;   bool velBool = false;   bool chanBool = false;
  String noteVal = "";     String velVal = "";     String chanVal = "";
  
  //Just Prints The Sender MAC Address
  Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  //Loops Though All The Messages Data
  for (int i = 0; i < static_cast<int>(count); ++i) {
    //Prints The Current Value
    Serial.print(static_cast<char>(buf[i]));

    //Checks If The Current Message Char Is A Digit Or Not
    if(isDigit(static_cast<char>(buf[i]))){
      if(noteBool == true){
        noteVal += static_cast<char>(buf[i]);
      }
      if(velBool == true){
        velVal += static_cast<char>(buf[i]);
      }
      if(chanBool == true){
        chanVal += static_cast<char>(buf[i]);
      }
    }else{
      //Works Out What String To Write To
      if(static_cast<char>(buf[i]) == 'N'){
        noteBool = true;
        velBool = false;
        chanBool = false;
      }
      if(static_cast<char>(buf[i]) == 'V'){
        noteBool = false;
        velBool = true;
        chanBool = false;
      }
      if(static_cast<char>(buf[i]) == 'C'){
        noteBool = false;
        velBool = false;
        chanBool = true;
      }
    }
  }
  
  //Finishes The Line In The Serial Monitor
  Serial.println();

  //Plays The MIDI Note
  MIDI.sendNoteOn(noteVal.toInt(), velVal.toInt(), chanVal.toInt());
}

void setup(){
  Serial.begin(115200);

  MIDI.begin(MIDI_CHANNEL_OMNI);

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

  ok = WifiEspNow.addPeer(PEER1);
  if (!ok) {
    Serial.println("WifiEspNow.addPeer() failed");
    ESP.restart();
  }
}

void loop(){

}

//Sends A Message To All Instruments To Light Up
void ActivateLights(){
  //Creates A Buffer With A Szie Of 60
  char msg[60];
  //Writes The Info To Buffer Whilst Getting It's Size
  int len = snprintf(msg, sizeof(msg), "Light_On");

  //Sends The Message Via The WIFIESPNOW Libary
  WifiEspNow.send(PEER1, reinterpret_cast<const uint8_t*>(msg), len);
}
