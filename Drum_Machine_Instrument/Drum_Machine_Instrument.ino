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

//Keypad Libary
#include <Keypad.h>

//Relay Pins
#define RelayPin 0

//Ultrasonic Sensor Pins
const int trigPin = 1;
const int echoPin = 3;

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

//Keypad Setup
const byte ROWS = 3;
const byte COLS = 3;

char keys[ROWS][COLS] = {{'0','1','2'},
                         {'3','4','5'},
                         {'6','7','8'}};
                         
byte rowPins[ROWS] = {5, 4, 2};
byte colPins[COLS] = {14, 12, 13};

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//Max Size For All Riffs
#define RiffSize 4

//Kick Riff Vars
long lastKickPlayedTime = 0;
int nextKickRiffNote = 0;

//Kick Pin 1 Riff
RiffNote kickRiffOne[RiffSize] {RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478), RiffNote(125, 50, 1, 478)};

//Kick Pin 2 Riff
RiffNote kickRiffTwo[RiffSize] {RiffNote(125, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329)};

//Kick Pin 3 Riff
RiffNote kickRiffThree[RiffSize] {RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188), RiffNote(117, 50, 1, 188)};

//Snare Riff Vars
long lastSnarePlayedTime = 0;
int nextSnareRiffNote = 0;

//Snare Pin 1 Riff
RiffNote snareRiffOne[RiffSize] {RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478), RiffNote(117, 50, 1, 478)};

//Snare Pin 2 Riff
RiffNote snareRiffTwo[RiffSize] {RiffNote(-1, 50, 1, 329), RiffNote(-1, 50, 1, 329), RiffNote(127, 50, 1, 329), RiffNote(-1, 50, 1, 329)};

//Snare Pin 3 Riff
RiffNote snareRiffThree[RiffSize] {RiffNote(-1, 50, 1, 188), RiffNote(-1, 50, 1, 188), RiffNote(127, 50, 1, 188), RiffNote(-1, 50, 1, 188)};

//Cymbal Riff Vars
long lastCymbalPlayedTime = 0;
int nextCymbalRiffNote = 0;

//Cymball Pin 1 Riff
RiffNote cymbalRiffOne[RiffSize] {RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478), RiffNote(123, 50, 1, 478)};

//Cymball Pin 2 Riff
RiffNote cymbalRiffTwo[RiffSize] {RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329), RiffNote(123, 50, 1, 329)};

//Cymball Pin 3 Riff
RiffNote cymbalRiffThree[RiffSize] {RiffNote(123, 50, 1, 188), RiffNote(123, 50, 1, 188), RiffNote(-1, 50, 1, 188), RiffNote(123, 50, 1, 188)};

//The State Of Each "Key"
int keyReadStates[ROWS * COLS] = {HIGH,   //KP1
                                  HIGH,   //SP1
                                  HIGH,   //CP1
                                  HIGH,   //KP2
                                  HIGH,   //SP2
                                  HIGH,   //CP2
                                  HIGH,   //KP3
                                  HIGH,   //SP3
                                  HIGH};  //CB3

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

  //Relay Pin Setup
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);

  //Ultrasonic Pin Setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Calibate Pin Setup
  pinMode(CalibratePin, INPUT_PULLUP);
}

void loop(){
  if (kpd.getKeys()){
    for (int i=0; i<LIST_MAX; i++){
      Serial.println(kpd.key[i].kchar);
      if (kpd.key[i].stateChanged){
        switch (kpd.key[i].kstate){
          case PRESSED:
            keyReadStates[kpd.key[i].kchar - '0'] = LOW;
            break;
          case HOLD:
            keyReadStates[kpd.key[i].kchar - '0'] = LOW;
            break;
          case RELEASED:
            keyReadStates[kpd.key[i].kchar - '0'] = HIGH;
            break;
          case IDLE:
            keyReadStates[kpd.key[i].kchar - '0'] = HIGH;
            break;
        }
      }
    }
  }
  
  //Kick Riff 1
  if(keyReadStates[0] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffOne, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Snare Riff 1
  if(keyReadStates[1] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffOne, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Cymbal Riff 1
  if(keyReadStates[2] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffOne, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Kick Riff 2
  if(keyReadStates[3] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffTwo, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Snare Riff 2
  if(keyReadStates[4] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffTwo, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Cymbal Riff 2
  if(keyReadStates[5] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffTwo, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Kick Riff 3
  if(keyReadStates[6] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(kickRiffThree, nextKickRiffNote, lastKickPlayedTime);
    nextKickRiffNote = ret.nextNote;
    lastKickPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Snare Riff 3
  if(keyReadStates[7] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(snareRiffThree, nextSnareRiffNote, lastSnarePlayedTime);
    nextSnareRiffNote = ret.nextNote;
    lastSnarePlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
  }

  //Cymbal Riff 3
  if(keyReadStates[8] == LOW){
    CancelCalibration();
    
    PlayDrumRiffReturner ret = PlayDrumRiff(cymbalRiffThree, nextCymbalRiffNote, lastCymbalPlayedTime);
    nextCymbalRiffNote = ret.nextNote;
    lastCymbalPlayedTime = ret.nextPlayTime;

    SyncPins(keyReadStates, ret.nextNote, ret.nextPlayTime);
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
void SyncPins(int krStates[], int nextNote, long nextPlayTime){
  //All Pins Must Be HIGH (Not Connected) Otherwise It'll Be Updated Twice And Then Sends It All Out Of Sync
  if(krStates[0] == HIGH && krStates[3] == HIGH && krStates[6] == HIGH){
    nextKickRiffNote = nextNote;
    lastKickPlayedTime = nextPlayTime;
  }
  if(krStates[1] == HIGH && krStates[4] == HIGH && krStates[7] == HIGH){
    nextSnareRiffNote = nextNote;
    lastSnarePlayedTime = nextPlayTime;
  }
  if(krStates[2] == HIGH && krStates[5] == HIGH && krStates[8] == HIGH){
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
