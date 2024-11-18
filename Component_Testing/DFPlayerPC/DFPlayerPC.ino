#include "mp3tf16p.h"

MP3Player mp3(0,1);

void setup() {
  Serial.begin(9600);
  mp3.initialize();
  // mp3.playTrackNumber(1, 30);
  // mp3.playTrackNumber(2,30);
  mp3.playTrackNumber(3,30);
}

void loop() {
  mp3.serialPrintStatus(MP3_ALL_MESSAGE);
}
