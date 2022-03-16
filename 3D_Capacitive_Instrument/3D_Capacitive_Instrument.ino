#include <CapacitiveSensor.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

//680k resistor between pins 16 & 5, pin 5 is sensor pin
CapacitiveSensor   cs1 = CapacitiveSensor(16,5);

void setup()                    
{
   Serial.begin(9600);
   MIDI.begin(MIDI_CHANNEL_OFF);
}

void loop()                    
{
    long start = millis();
    long total1 =  cs1.capacitiveSensor(30);

    Serial.print(millis() - start);
    Serial.print("\t|\t");

    Serial.print(total1);

    if(total1 > 25){
      Serial.println("\t|\tDrum Noise!");
      MIDI.sendNoteOn(36, 127, 1);
    }else{
      Serial.println("\t|\tSilence");
    }

    delay(250);
    
    if(total1 > 25){
      MIDI.sendNoteOn(36, 0, 1);
    }
}
