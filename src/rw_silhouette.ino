/*
  SD card read/write
 
 This example shows how to read and write data to and from an SD card file 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	 
 */
 
#include "FastLED.h"
#include <SD.h>

Sd2Card card;
File myFile;

//struct silhouette {
//  uint8_t params;
//  int frames_nbr;
//  CRGB frames[][60];
//};
struct silhouette {
  uint8_t frames[][60][3];
};

void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  silhouette sil;  
  
  //sil.frames[0][0] = CRGB(0,0,0);
  //sil.frames[0][1] = CRGB(1,1,1);
  //sil.frames[0][2] = CRGB(2,2,2);
  //sil.frames[1][0] = CRGB(3,3,3);
  //sil.frames[1][1] = CRGB(4,4,4);
  //sil.frames[1][2] = CRGB(5,5,5);
  //Serial.println(sizeof(sil));
  //Serial.println(sizeof(sil.frames[0]));
  
  //sil.frames[0] = CRGB(0,0,0);
  //sil.frames[1] = CRGB(5,6,7);
  
  
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
   
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  SD.remove("test.txt");
  myFile = SD.open("test.txt", FILE_WRITE);
  
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test2.txt...");
    myFile.write(128);
    myFile.write((byte) 0);
    myFile.write(1);
    myFile.write((byte) 128);
    myFile.write((byte) 0);
    myFile.write(255);
    for (int i=0; i<59; i++) {
      myFile.write((byte) 0);
      myFile.write((byte) 0);
      myFile.write((byte) 0);
    }
    
    /*
    for (int i=0; i<60; i++) {
      myFile.write(i);
      myFile.write(1);
      myFile.write(2);
    }
    for (int i=0; i<60; i++) {
      myFile.write(254);
      myFile.write(255);
      myFile.write(255-i);
    }
    for (int i=0; i<60; i++) {
      myFile.write(127);
      myFile.write(128);
      myFile.write(129);
    }
    */
    //myFile.write((byte *) &sil, 6);       // this works for struct CRGB frames[]
	// close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test2.txt");
  }
  /*
  // re-open the file for reading:
  myFile = SD.open("test4.txt");
  if (myFile) {
    Serial.println("test2.txt:");
    
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
    	Serial.print(myFile.read(), HEX);
    }
    // close the file:
    myFile.close();
  } else {
  	// if the file didn't open, print an error:
    Serial.println("error opening test2.txt");
  }
  */
}

void loop()
{
	// nothing happens after setup
}


