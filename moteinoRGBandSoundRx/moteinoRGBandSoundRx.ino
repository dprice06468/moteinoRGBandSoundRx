// change color of common anode RGB LED based on serial input
// $R - fade to red
// $G - fade to green
// $B - fade to blue
// $W - fade to white
// $K - fade to black
//
// connect an RGB LED with common anode to the following PWM pins
// set FADESPEED to a higher value to slow the fade

#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        1    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************

//********** set up soundFX board **************************************
// define SoundFX pins
#define SFX_TX  9
#define SFX_RX  7
#define SFX_RST 4

// we'll be using software serial
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);

// pass the software serial to Adafruit_soundboard, the second
// argument is the debug port (not used really) and the third 
// arg is the reset pin
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);
//*********************************************************************************************

#define SERIAL_BAUD   115200

#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network

// define RGB pinouts
#define REDPINOUT 3
#define GREENPINOUT 5
#define BLUEPINOUT 6
 
// make this higher to slow down the fade
#define FADESPEED 10     

// set the initial color to black
int iRedLevel = 255;
int iGreenLevel = 255;
int iBlueLevel = 255;

//------------------------------------------------------------------
void setup() {
  Serial.println("moteinoRGBandSoundRx");
  Serial.begin(SERIAL_BAUD);
  ss.begin(9600);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower(); //only for RFM69HW!
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  //radio.setFrequency(919000000); //set frequency to some custom frequency
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
    
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif

  //setup RGB output pins
  pinMode(REDPINOUT, OUTPUT);
  pinMode(GREENPINOUT, OUTPUT);
  pinMode(BLUEPINOUT, OUTPUT);

  // set the initial color to black
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  Serial.println("should be black");

  if (!sfx.reset()) {
    Serial.println("SFX board not found");
    while (1);
  }
  Serial.println("SFX board found");
}

uint32_t packetCount = 0;

//------------------------------------------------------------------
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char charData[20];  //to hold char data received
    String strData;     //to hold string data received

    strData = Serial.readString();

    Serial.print("==> "); Serial.println(strData);

    strData.toCharArray(charData, 20);
    switch (charData[0]) {
      case '%':
        changeColor(strData);
        break;
      case '$':
//        processColor(charData);
//        splitString(charData, ',');
  const char d[2] = ",";
  char * token;

  token = strtok(charData, d);

  while (token != NULL) {
    //process token
    Serial.println(token);

    //get the next token
    token = strtok(NULL, d);
  }

        for (byte j = 1; j < strData.length(); j++) {
          Serial.println("processing char " + (String)j + ": " + charData[j]);
          changeColor(charData[j]);
        }
        break;
      case '#':
        playSound(strData);
        break;
    }
  }

  if (radio.receiveDone())
  {
    char charData[20];  //to hold char data received
    String strData;     //to hold string data received
    byte i;
    
    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");

    for (i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
      charData[i] = (char)radio.DATA[i];
    }
    charData[i] = '\0';
    strData = String(charData);
    Serial.print("==> "); Serial.println(strData);
    
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    switch (charData[0]) {
      case '%':
        changeColor(strData);
        break;
      case '$':
        for (byte j = 1; j < strData.length(); j++) {
          Serial.println("processing char " + (String)j + ": " + charData[j]);
          changeColor(charData[j]);
        }
        break;
      case '#':
        playSound(strData);
        break;
    }
    
    Serial.println();
    Blink(LED,3);
  }
}

//------------------------------------------------------------------
String splitString(String input, char chr) {
  char chrarray[25];
  const char d[2] = ",";
  char * token;

  input.toCharArray(chrarray, 25);
  token = strtok(chrarray, d);

  while (token != NULL) {
    //process token
    Serial.println(token);
    token = strtok(NULL, d);
  }
//  for (int i = 0; i < input.length(); i++) {
//    if (input.substring(i, i+1) == ",") {
//      firstVal = input.substring(0, i).toInt();
//      secondVal = input.substring(i+1).toInt();
//      break;
//    }
//  }
}

//------------------------------------------------------------------
void processColor(String strColors) {
  //String strClrCmds[30];

//  strClrCmds = strColors.split
}

//------------------------------------------------------------------
void playSound(String strSound) {
  Serial.println("in playSound: " + strSound);
  
  char cmd[5];
  strSound.toCharArray(cmd, 5);
  
  switch (cmd[1]) {
    case 'r': {
      if (!sfx.reset()) {
        Serial.println("Reset failed");
      }
      break; 
    }
    
    case 'L': {
      uint8_t files = sfx.listFiles();
    
      Serial.println("File Listing");
      Serial.println("========================");
      Serial.println();
      Serial.print("Found "); Serial.print(files); Serial.println(" Files");
      for (uint8_t f=0; f<files; f++) {
        Serial.print(f); 
        Serial.print("\tname: "); Serial.print(sfx.fileName(f));
        Serial.print("\tsize: "); Serial.println(sfx.fileSize(f));
      }
      Serial.println("========================");
      break; 
    }
    
    case '#': {
      int i = cmd[2] - '0';
      Serial.print("Playing track #"); Serial.println(i);
      if (! sfx.playTrack(i) ) {
        Serial.println("Failed to play track?");
      }
      break;
    }
    
    case '=': {
      Serial.println("Pausing...");
      if (! sfx.pause() ) Serial.println("Failed to pause");
      break;
    }
   
    case '>': {
      Serial.println("Unpausing...");
      if (! sfx.unpause() ) Serial.println("Failed to unpause");
      break;
    }
   
    case 'q': {
      Serial.println("Stopping...");
      if (! sfx.stop() ) Serial.println("Failed to stop");
      break;
    }  

  }
}

//------------------------------------------------------------------
void changeColor(String strColor) {
  byte r = strColor.indexOf('r');
  byte g = strColor.indexOf('g');
  byte b = strColor.indexOf('b');

  String strRVal = strColor.substring(r+1,g);
  String strGVal = strColor.substring(g+1,b);
  String strBVal = strColor.substring(b+1);

  Serial.println("changing color to r:" + strRVal + ",g:" + strGVal + ",b:" + strBVal);
  
  iRedLevel = 255 - strRVal.toInt();
  iGreenLevel = 255 - strGVal.toInt();
  iBlueLevel = 255 - strBVal.toInt();
  analogWrite(REDPINOUT, iRedLevel);
  analogWrite(GREENPINOUT, iGreenLevel);
  analogWrite(BLUEPINOUT, iBlueLevel);
}

//------------------------------------------------------------------
void changeColor(char input) {
  switch (input) {
    case 'k':
      Serial.println("instant black");
      instantBlack();
      Serial.println("should be black");
      break;
    case 'K':
      Serial.println("fading to black");
      fadeToBlack();
      Serial.println("should be black");
      break;
    case 'r':
      Serial.println("instant red");
      instantRed();
      Serial.println("should be red");
      break;
    case 'R':
      Serial.println("fading to red");
      fadeToRed();
      Serial.println("should be red");
      break;
    case 'g':
      Serial.println("instant green");
      instantGreen();
      Serial.println("should be green");
      break;
    case 'G':
      Serial.println("fading to green");
      fadeToGreen();
      Serial.println("should be green");
      break;
    case 'b':
      Serial.println("instant blue");
      instantBlue();
      Serial.println("should be blue");
      break;
    case 'B':
      Serial.println("fading to blue");
      fadeToBlue();
      Serial.println("should be blue");
      break;
    case 'w':
      Serial.println("instant white");
      instantWhite();
      Serial.println("should be white");
      break;
    case 'W':  
      Serial.println("fading to white");
      fadeToWhite();
      Serial.println("should be white");
      break;
    case 't':
      {
        byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
        byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
        Serial.print( "Radio Temp is ");
        Serial.print(temperature);
        Serial.print("C, ");
        Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
        Serial.println('F');
        break;
      }
  }
}

//------------------------------------------------------------------
void instantBlack() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 255;
  iGreenLevel = 255;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToBlack() {
  while (iRedLevel < 255 | iGreenLevel < 255 | iBlueLevel < 255) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantBlack();
}

//------------------------------------------------------------------
void instantRed() {
  digitalWrite(REDPINOUT, LOW);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 0;
  iGreenLevel = 255;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToRed() {
  while (iRedLevel > 0 | iGreenLevel < 255 | iBlueLevel < 255) {
    Serial.println(iRedLevel);
    iRedLevel = (iRedLevel > 0) ? iRedLevel-1 : 0;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantRed();
}

//------------------------------------------------------------------
void instantGreen() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, LOW);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 255;
  iGreenLevel = 0;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToGreen() {
  while (iRedLevel < 255 | iGreenLevel > 0 | iBlueLevel < 255) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel > 0) ? iGreenLevel-1 : 0;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantGreen();
}

//------------------------------------------------------------------
void instantBlue() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, LOW);
  iRedLevel = 255;
  iGreenLevel = 255;
  iBlueLevel = 0;
}

//------------------------------------------------------------------
void fadeToBlue() {
  while (iRedLevel < 255 | iGreenLevel < 255 | iBlueLevel > 0) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel > 0) ? iBlueLevel-1 : 0;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantBlue();
}

//------------------------------------------------------------------
void instantWhite() {
  digitalWrite(REDPINOUT, LOW);
  digitalWrite(GREENPINOUT, LOW);
  digitalWrite(BLUEPINOUT, LOW);
  iRedLevel = 0;
  iGreenLevel = 0;
  iBlueLevel = 0;
}

//------------------------------------------------------------------
void fadeToWhite() {
  while (iRedLevel > 0 | iGreenLevel > 0 | iBlueLevel > 0) {
    iRedLevel = (iRedLevel > 0) ? iRedLevel-1 : 0;
    iGreenLevel = (iGreenLevel > 0) ? iGreenLevel-1 : 0;
    iBlueLevel = (iBlueLevel > 0) ? iBlueLevel-1 : 0;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  //instantWhite();
}

//------------------------------------------------------------------
void Blink(byte PIN, int DELAY_MS) {
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
