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
#define RiffSize 4

//Kick Riff Vars
long lastKickPlayedTime = 0;
int nextKickRiffNote = 0;

//Kick Pin 1 Riff
#define KickPin1 5
RiffNote kickRiffOne[RiffSize] {RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478)};

//Kick Pin 2 Riff
#define KickPin2 1
RiffNote kickRiffTwo[RiffSize] {RiffNote(125, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329)};

//Kick Pin 3 Riff
#define KickPin3 10
RiffNote kickRiffThree[RiffSize] {RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188)};

//Snare Riff Vars
long lastSnarePlayedTime = 0;
int nextSnareRiffNote = 0;

//Snare Pin 1 Riff
#define SnarePin1 13
RiffNote snareRiffOne[RiffSize] {RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478)};

//Snare Pin 2 Riff
#define SnarePin2 4
RiffNote snareRiffTwo[RiffSize] {RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(127, 50, 1, 329), RiffNote(-1, 50, 1, 329)};

//Snare Pin 3 Riff
#define SnarePin3 9
RiffNote snareRiffThree[RiffSize] {RiffNote(-1, 50, 1, 188), RiffNote(-1, 50, 1, 188), RiffNote(127, 50, 1, 188), RiffNote(-1, 50, 1, 188)};

//Cymbal Riff Vars
long lastCymbalPlayedTime = 0;
int nextCymbalRiffNote = 0;

//Cymball Pin 1 Riff
#define CymbalPin1 3
RiffNote cymbalRiffOne[RiffSize] {RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478)};

//Cymball Pin 2 Riff
#define CymbalPin2 2
RiffNote cymbalRiffTwo[RiffSize] {RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329)};

//Cymball Pin 3 Riff
#define CymbalPin3 8
RiffNote cymbalRiffThree[RiffSize] {RiffNote(123, 50, 1, 188), RiffNote(123, 50, 1, 188), RiffNote(-1, 50, 1, 188), RiffNote(123, 50, 1, 188)};

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

  //Start The EEPROM
  EEPROM.begin(16);

  //Check The EEPROM For Data
  if(EEPROM.percentUsed() > 0){
    EEPROM.get(0, maxLength);
  }

  //Ultrasonic Pin Setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Calibate Pin Setup
  pinMode(CalibratePin, INPUT_PULLUP);

  //Drum Pin Setup
  pinMode(KickPin1, INPUT_PULLUP);
  pinMode(SnarePin1, INPUT_PULLUP);
  pinMode(CymbalPin1, INPUT_PULLUP);
  pinMode(KickPin2, INPUT_PULLUP);
  pinMode(SnarePin2, INPUT_PULLUP);
  pinMode(CymbalPin2, INPUT_PULLUP);
}

void loop(){
  int kp1Read = digitalRead(KickPin1);
  int sp1Read = digitalRead(SnarePin1);
  int cp1Read = digitalRead(CymbalPin1);

  int kp2Read = digitalRead(KickPin2);
  int sp2Read = digitalRead(SnarePin2);
  int cp2Read = digitalRead(CymbalPin2);
  
  if(kp1Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffOne, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
  }

  if(sp1Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffOne, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
  }

  if(cp1Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffOne, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
  }

  if(kp2Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffTwo, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
  }
  
  if(sp2Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffTwo, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
  }

  if(cp2Read == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffTwo, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;

    SyncPins(kp1Read, sp1Read, cp1Read, kp2Read, sp2Read, cp2Read, ret.nextNote, ret.nextPlayTime);
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
    }

    lastReadState = calPinRead;
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
        SendMIDIMSG(riffArray[nextRiffNote].noteVal, int(map(GetUFDistance(), 0, maxLength, 0, 127)), riffArray[nextRiffNote].chanVal, riffArray[nextRiffNote].timeVal);
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
        SendMIDIMSG(riffArray[nextRiffNote].noteVal, int(map(GetUFDistance(), 0, maxLength, 0, 127)), riffArray[nextRiffNote].chanVal, riffArray[nextRiffNote].timeVal);
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

//Keeps The Riffs In Line So They Can Be Played In Sync No Matter The Connection Time
void SyncPins(int kp1Read, int sp1Read, int cp1Read, int kp2Read, int sp2Read, int cp2Read, int nextNote, long nextPlayTime){
  //Both Pins Must Be HIGH (Not Connected) Otherwise It'll Be Updated Twice And Then Sends It All Out Of Sync
  if(kp1Read == HIGH && kp2Read == HIGH){
    nextKickRiffNote = nextNote;
    lastKickPlayedTime = nextPlayTime;
  }
  if(sp1Read == HIGH && sp2Read == HIGH){
    nextSnareRiffNote = nextNote;
    lastSnarePlayedTime = nextPlayTime;
  }
  if(cp1Read == HIGH && cp2Read == HIGH){
    nextCymbalRiffNote = nextNote;
    lastCymbalPlayedTime = nextPlayTime;
  }
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

void CancelCalibration(){
  if(calibrateable == true){
    calibrateable = false;
  }
}
