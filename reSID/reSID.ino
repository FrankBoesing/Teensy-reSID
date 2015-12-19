
//#include "tower.h"
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "reSID.h"


#define DMPDIR "/dmp"

//#define DMP "/dmp/GHOSTB~1.DMP"
//#define DMP "/dmp/LAX_UP.DMP"
#define DMP "/dmp/MR_MAR~1.DMP"


// GUItool: begin automatically generated code
AudioPlaySID             playSID;  //xy=189,110
AudioOutputI2S           i2s1;           //xy=366,111
AudioConnection          patchCord1(playSID, 0, i2s1, 0);
AudioConnection          patchCord2(playSID, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=354,176
// GUItool: end automatically generated code


const int SDchipSelect = 10;
File directory;
File myfile;

char buffer[26];
char oldbuffer[26];


void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}


void setup() {

#if USETOWER
  initTower();
#endif  
  
  AudioMemory(10);
  
  SPI.setSCK(14);      
  SPI.setMOSI(7);
  SPI.setMISO(12);
  SPI.begin();
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);
  sgtl5000_1.enhanceBassEnable();
  sgtl5000_1.enhanceBass(0.5, 2.5);

  memset(buffer,0,sizeof(buffer));
  memset(oldbuffer,0,sizeof(oldbuffer));
  
  int ml = millis();
  while (!Serial && (millis()-ml < 1000)) ; // wait for Arduino Serial Monitor
  
  Serial.print("Initializing SD card...");

  if (!SD.begin(SDchipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  //directory = SD.open(DMPDIR);

  //printDirectory(directory, 1);

  myfile = SD.open(DMP, FILE_READ);

}


void loop() {
    static int m = millis();
    if (millis()-m < 20) return;
    m = millis();
        
    if (!myfile.available()) return;   
    for(int i=0;i<25;i++) {
      if(buffer[i] != oldbuffer[i]) {       
        playSID.setreg(i, buffer[i]);
        oldbuffer[i] = buffer[i];                  
      } 

    }

    myfile.read(buffer, 25);

}



