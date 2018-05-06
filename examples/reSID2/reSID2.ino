

//Attention, compile with >=144MHz, -O2 


//#include "tower.h"
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "reSID.h"

#define DMP  "/dmp/GHOSTB~1.DMP"
#define DMP2 "/dmp/pp_ham~1.DMP"

// GUItool: begin automatically generated code
AudioPlaySID             playSID;  //xy=189,110
AudioPlaySID             playSID1;  //xy=189,110
AudioOutputI2S           i2s1;           //xy=366,111
AudioConnection          patchCord1(playSID, 0, i2s1, 0);
AudioConnection          patchCord2(playSID1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=354,176
// GUItool: end automatically generated code


const int SDchipSelect = 10;

File myfile;
File myfile2;

char buffer[26];
char oldbuffer[26];
char oldbuffer2[26];


void setup() {

#if USETOWER
  initTower();
#endif  
  
  AudioMemory(4);
  
  SPI.setSCK(14);      
  SPI.setMOSI(7);
  SPI.setMISO(12);
  SPI.begin();
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8);
  sgtl5000_1.enhanceBassEnable();
  sgtl5000_1.enhanceBass(0.5, 2.5);

  memset(oldbuffer,0,sizeof(oldbuffer));
  
  int ml = millis();
  while (!Serial && (millis()-ml < 1000)) ; // wait for Arduino Serial Monitor
  
  Serial.print("Initializing SD card...");

  if (!SD.begin(SDchipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  myfile = SD.open(DMP, FILE_READ);
  myfile2 = SD.open(DMP2, FILE_READ);

}


void loop() {
    static int m = millis();
    if (millis()-m < 20) return;
    m = millis();
        

    //Play first file
    if (myfile.available()) {
      myfile.read(buffer, 25);
      for(int i=0;i<25;i++) {
        if(buffer[i] != oldbuffer[i]) {       
           playSID.setreg(i, buffer[i]);
          oldbuffer[i] = buffer[i];                  
        } 
      }
    } else {
      if (playSID.isPlaying()) playSID.stop();
    }

    //Play second file
    if (myfile2.available()) {
      myfile2.read(buffer, 25);
      for(int i=0;i<25;i++) {
        if(buffer[i] != oldbuffer2[i]) {                
           playSID1.setreg(i, buffer[i]);
          oldbuffer2[i] = buffer[i];                  
        } 
      }
    } else {
      if (playSID1.isPlaying()) playSID1.stop();
    }
      
}



