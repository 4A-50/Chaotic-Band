//WIFI ESP NOW Libary
#include <WifiEspNow.h>

//Picks The Right WIFI Libary For ESP Architecture
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

//The Instrument Mac Addresses
//THIS NEEDS FIXING, MAYBE ARRAY OF PEER CLASS?
static uint8_t PEERS[1][6]{{0x42, 0x91, 0x51, 0x51, 0x70, 0x5F},
                           {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

//
void printReceivedMessage(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg){
  Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  for (int i = 0; i < static_cast<int>(count); ++i) {
    Serial.print(static_cast<char>(buf[i]));
  }
  Serial.println();
}

void setup(){
  Serial.begin(115200);
  Serial.println();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP("ESPNOW", nullptr, 3);
  WiFi.softAPdisconnect(false);
  // WiFi must be powered on to use ESP-NOW unicast.
  // It could be either AP or STA mode, and does not have to be connected.
  // For best results, ensure both devices are using the same WiFi channel.

  Serial.print("MAC address of this node is ");
  Serial.println(WiFi.softAPmacAddress());

  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  Serial.println();
  Serial.println("You can paste the following into the program for the other device:");
  Serial.printf("static uint8_t PEER[]{0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X};\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println();

  bool ok = WifiEspNow.begin();
  if (!ok) {
    Serial.println("WifiEspNow.begin() failed");
    ESP.restart();
  }

  WifiEspNow.onReceive(printReceivedMessage, nullptr);

  ok = WifiEspNow.addPeer(PEER);
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

  //Loops Through All The MAC Adresses
  for(int i = 0; i < PEERS.length; i++){
    //Sends The Message Via The WIFIESPNOW Libary
    WifiEspNow.send(PEERS[i], reinterpret_cast<const uint8_t*>(msg), len);
  }
}
