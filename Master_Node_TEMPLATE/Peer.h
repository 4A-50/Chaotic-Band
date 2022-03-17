#ifndef Morse_h
#define Morse_h

#include "Arduino.h"

class Peer{
  public:
    Peer(uint8_t _peer[6]);
    uint8_t PEER[6];
};

#endif
