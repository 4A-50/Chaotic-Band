//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif


const int trigPin = 12;
const int echoPin = 14;

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

//RiffNote Class
class RiffNote{
  public:
    int noteVal;
    int timeVal;
    int chanVal;
    int pauseTime;

    RiffNote(int _nV, int _tV, int _cV, int _pT){
      noteVal = _nV;
      timeVal = _tV;
      chanVal = _cV;
      pauseTime = _pT;
    }
};

//Struct To Return Info
struct PlayDrumRiffReturner{
  int nextNote;
  long nextPlayTime;
};

//Max Size For All Riffs
#define RiffSize 3

//Kick Riff Vars
int nextKickRiffNote = 0;
long lastKickPlayedTime = 0;

//Kick Pin 1 Riff
#define KickPin1 5
RiffNote kickRiffOne[RiffSize] {RiffNote(36, 50, 1, 100), RiffNote(36, 50, 1, 100), RiffNote(-1, 50, 1, 250)};

//Snare Riff Vars
int nextSnareRiffNote = 0;
long lastSnarePlayedTime = 0;

//Snare Pin 1 Riff
#define SnarePin1 13
RiffNote snareRiffOne[RiffSize] {RiffNote(-1, 50, 1, 100), RiffNote(-1, 50, 1, 100), RiffNote(38, 50, 1, 250)};

//Cymbal Riff Vars
int nextCymbalRiffNote = 0;
long lastCymbalPlayedTime = 0;

//Cymball Pin 1 Riff
#define CymbalPin1 15
RiffNote cymbalRiffOne[RiffSize] {RiffNote(-1, 50, 1, 100), RiffNote(-1, 50, 1, 100), RiffNote(49, 50, 1, 250)};

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
    //Turn The LEDS ON
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

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Set Up The Drum Pins
  pinMode(KickPin1, INPUT_PULLUP);
  pinMode(SnarePin1, INPUT_PULLUP);
  pinMode(CymbalPin1, INPUT_PULLUP);
}

void loop(){
  if(digitalRead(KickPin1) == LOW){
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffOne, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;
  }

  if(digitalRead(SnarePin1) == LOW){
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffOne, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;
  }

  if(digitalRead(CymbalPin1) == LOW){
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffOne, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;
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

PlayDrumRiffReturner PlayDrumRiff(RiffNote riffArray[], int nextRiffNote, long lastPlayedTime){
  if(nextRiffNote - 1 > 0){
    if(millis() > lastPlayedTime + riffArray[nextRiffNote - 1].pauseTime){
      if(riffArray[nextRiffNote].noteVal != -1){
        SendMIDIMSG(riffArray[nextRiffNote].noteVal, GetUFDistance(), riffArray[nextRiffNote].chanVal, riffArray[nextRiffNote].timeVal);
      }
      
      lastPlayedTime = millis();
      
      if(nextRiffNote + 1 >= RiffSize){
        nextRiffNote = 0;
      }else{
        nextRiffNote++;
      }
    }
  }else{
    if(millis() > lastPlayedTime + riffArray[RiffSize - 1].pauseTime){
      if(riffArray[nextRiffNote].noteVal != -1){
        SendMIDIMSG(riffArray[nextRiffNote].noteVal, GetUFDistance(), riffArray[nextRiffNote].chanVal, riffArray[nextRiffNote].timeVal);
      }
      
      lastPlayedTime = millis();
      
      if(nextRiffNote + 1 >= RiffSize){
        nextRiffNote = 0;
      }else{
        nextRiffNote++;
      }
    }
  }

  PlayDrumRiffReturner retVal = {nextRiffNote, lastPlayedTime};
  return retVal;
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

  return int(map(distance, 0, 32, 0, 127));
}
