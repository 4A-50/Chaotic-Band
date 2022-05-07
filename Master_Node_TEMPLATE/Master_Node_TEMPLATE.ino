//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//The MIDI Libary
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

//The Peer Class
class Peer{
  public:
    uint8_t PEER[6];

    Peer(uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6){
      PEER[0] = _1;
      PEER[1] = _2;
      PEER[2] = _3;
      PEER[3] = _4;
      PEER[4] = _5;
      PEER[5] = _6;
    }
};

//List Libary
#include <List.hpp>

//PlayingNote Class
class PlayingNote{
  public:
    int noteVal;
    int timeVal;
    int chanVal;

    PlayingNote(int _nV, int _tV, int _cV){
      noteVal = _nV;
      timeVal = _tV;
      chanVal = _cV;
    }
};

List<PlayingNote> playingNotes;

//The Time At Which The Last Note Was Played
long lastNoteTime;
//The State Of The Lights
bool instLightsOn = false;

//The Instrument Mac Addresses
#define PEERLength 2
static Peer PEERS[PEERLength]{Peer(0x42, 0x91, 0x51, 0x51, 0x70, 0x5F), //Drum Machine
                              Peer(0x5A, 0xBF, 0x25, 0xD7, 0x42, 0x2B), //Distance Bass
                              Peer(0x42, 0x91, 0x51, 0x44, 0x92, 0x74)}; //Motion Sensor

//Decodes The Incoming Message And Plays The Correct MIDI Info
void MessageDecoder(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  //Var Holders To Write The Buffer Data Too
  bool noteBool = false;   bool velBool = false;   bool chanBool = false;   bool timeBool = false;
  String noteVal = "";     String velVal = "";     String chanVal = "";     String timeVal = "";

  //Loops Though All The Messages Data
  for (int i = 0; i < static_cast<int>(count); ++i) {

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
      if(timeBool == true){
        timeVal += static_cast<char>(buf[i]);
      }
    }else{
      //Works Out What String To Write To
      if(static_cast<char>(buf[i]) == 'N'){
        noteBool = true;
        velBool = false;
        chanBool = false;
        timeBool = false;
      }
      if(static_cast<char>(buf[i]) == 'V'){
        noteBool = false;
        velBool = true;
        chanBool = false;
        timeBool = false;
      }
      if(static_cast<char>(buf[i]) == 'C'){
        noteBool = false;
        velBool = false;
        chanBool = true;
        timeBool = false;
      }
      if(static_cast<char>(buf[i]) == 'T'){
        noteBool = false;
        velBool = false;
        chanBool = false;
        timeBool = true;
      }
    }
  }
  
  int inNoteVal = noteVal.toInt();
  int inVelVal = velVal.toInt();
  int inChanVal = chanVal.toInt();
  int inTimeVal = timeVal.toInt();

  if(inNoteVal == 500 && instLightsOn == false){
    LightControl(true);

    //Set's The MIDI Note To The Correct Value
    inNoteVal = 53;
    
    //Plays The MIDI Note
    MIDI.sendNoteOn(inNoteVal, inVelVal, inChanVal);
    lastNoteTime = millis();
  }else{
    //Plays The MIDI Note
    MIDI.sendNoteOn(inNoteVal, inVelVal, inChanVal);
    lastNoteTime = millis();
  }

  //If The Note Has An Off Time
  if(inTimeVal > 0){
    //Works Out The Off Time From Current Time
    inTimeVal = millis() + inTimeVal;

    //Adds The Note To The List
    PlayingNote newNote = PlayingNote(inNoteVal, inTimeVal, inChanVal);
    playingNotes.add(newNote);
  }
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

  //Loops Through All The PEER MAC Adresses
  for(int i = 0; i < PEERLength; i++){ 
    ok = WifiEspNow.addPeer(PEERS[i].PEER);
    if (!ok) {
      Serial.println("WifiEspNow.addPeer() failed");
      ESP.restart();
    }
  }
}

void loop(){
  //Checks That Their Are Notes That Need Stopping
  if(playingNotes.getSize() > 0){
    //Loop Through Them All Backwards (Easier To Remove Items This Way)
    for(int i = playingNotes.getSize() - 1; i >= 0; i--){
      //If The Notes Off Time Is Now
      if(playingNotes[i].timeVal == millis()){
        //Send The Off Note
        MIDI.sendNoteOn(playingNotes[i].noteVal, 0, playingNotes[i].chanVal);
        //Remove The Note From The List
        playingNotes.remove(i);
      }
    }
  }

  //Checks To See If The Lights Should Be Turned Off
  if(lastNoteTime + 30000 < millis() && instLightsOn == true){
    LightControl(false);
  }
}

//Sends A Message To All Instruments To Light Up
void LightControl(bool onOff){
  //Creates A Buffer With A Size Of 60
  char msg[60];
  //Writes The Info To Buffer Whilst Getting It's Size
  if(onOff == true){
    int len = snprintf(msg, sizeof(msg), "Light_On");
    
    instLightsOn = true;
  }else{
    int len = snprintf(msg, sizeof(msg), "Light_Off");
    
    instLightsOn = false;
  }

  //Loops Through All The PEER MAC Adresses
  for(int i = 0; i < PEERLength; i++){
    //Sends The Message Via The WIFIESPNOW Libary
    WifiEspNow.send(PEERS[i].PEER, reinterpret_cast<const uint8_t*>(msg), len);
  }
}
