#include "Arduino.h"
#include "Peer.h"

Peer::Peer(uint8_t _peer[6]){
  for(int i = 0; i < 6; i++){
    PEER[i] = _peer[i];
  }
}
