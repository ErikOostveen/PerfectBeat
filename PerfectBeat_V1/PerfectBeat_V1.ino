#include <Button.h>
#include <Adafruit_NeoPixel.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

// MIDI Info
// ------------------------------------------
// clock (decimal 248, hex 0xF8)
// start (decimal 250, hex 0xFA)
// continue (decimal 251, hex 0xFB)
// stop (decimal 252, hex 0xFC)
// ------------------------------------------
// Erik Oostveen
// June 2020
// www.erikoostveen.co.uk
// ------------------------------------------

int SendStart = 0;

// Control Nixie Tube
int pin_c = 8;
int pin_b = 7;
int pin_a = 6;

// Control LED bar
int latchPin = 9;
int clockPin = 10; 
int dataPin = 11; 
byte led = 0; 

int downBeatAndStart = 0;
int downBeatOnly = 0;

int counterDislay;

int MIDITicksCounter=0;

int SendClock = 1;

Button SetDownbeatAndStart(4);
Button SetDownbeatOnly(5);
Button ArmedStart(3);
Button StopNow(2);
Button slowDownButton(A0);
Button speedUpButton(A1);
Button randomLED(A2);
Button NixieCount(A4);

#define PIN 13 // NeoPixel PIN
#define NUMPIXELS 24 // Ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int colorR[] = {0,102,255,255,255,120,80,54,200,170,243,90,12,0,12,180,124,220,190,5};
int colorG[] = {0,255,103,0,255,30,80,180,55,200,220,212,89,113,200,66,31,170,240,110};
int colorB[] = {255,0,0,0,255,60,20,80,123,201,180,120,130,140,77,44,98,92,100,51};

int LEDcolor;
int LEDcolorRND;
int LEDnumber;
int LEDbrightness;

int randomLEDflag = 0;

int NixieMode = 0;

void setup() {

  // Armed Start LED
  pinMode(12, OUTPUT); 

  // Set all pins controlling 74141 as OUTPUT
  pinMode(pin_c, OUTPUT);
  pinMode(pin_b, OUTPUT);  
  pinMode(pin_a, OUTPUT);

  // Set all pins controlling 74HC595 as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  
  // Define buttons
  SetDownbeatAndStart.begin();
  SetDownbeatOnly.begin();
  ArmedStart.begin();
  StopNow.begin();
  slowDownButton.begin();
  speedUpButton.begin();
  randomLED.begin();
  NixieCount.begin();

  pixels.begin(); // INITIALIZE NeoPixel objec
    
      MIDI.begin(MIDI_CHANNEL_OMNI);
      MIDI.turnThruOff();
}

void loop() {

  if (SetDownbeatOnly.pressed()){
    downBeatOnly = 1;
   }

  if (SetDownbeatAndStart.pressed()){
    downBeatAndStart = 1;
   }

  if (ArmedStart.pressed()){
    SendStart = 1;
    digitalWrite(12, HIGH);
  }

  if (StopNow.pressed()){
    MIDI.sendStop();
  } 

 if (randomLED.toggled()) {
   if (randomLED.read() == Button::PRESSED)
      randomLEDflag = 1;
    else
      randomLEDflag = 0;
  }

 if (NixieCount.toggled()) {
   if (NixieCount.read() == Button::PRESSED)
      NixieMode = 0;
    else
      NixieMode = 1;
  }


  
        if (MIDI.read() > 0) {
        
              //IncomingMIDIByte = Serial.read();



                   // if ( IncomingMIDIByte == 159 ) { 
                    //Serial.write(250); 
                     //IncomingMIDIByte = 0; 
                     //downBeatOnly = 1;             
                   // }



                // Pass through all but MIDI clock bytes  
                //if ( IncomingMIDIByte != 248 ) {
                 // Serial.write(IncomingMIDIByte);              
                //}


            
                        switch(MIDI.getType())      // Get the type of the message we caught
                        {

                            case midi::NoteOn: // Detect note on
                              if ( MIDI.getChannel() == 16 && MIDI.getData1() == 62 ) // CH16, d5 note, set Downbeat - no start
                              {
                                downBeatOnly = 1;                                                         
                              }
                              if ( MIDI.getChannel() == 16 && MIDI.getData1() == 60 ) // CH6, c5 note, set Downbeat - AND start
                              {
                                downBeatAndStart = 1;                                                      
                              }
                              if ( MIDI.getChannel() == 16 && MIDI.getData1() == 64 ) // CH6, e5 note, STOP
                              {
                                MIDI.sendStop();                                                      
                              } 
                              if ( MIDI.getChannel() < 16 ) 
                              {
                                    MIDI.send(MIDI.getType(),
                                    MIDI.getData1(),
                                    MIDI.getData2(),
                                    MIDI.getChannel());                                                      
                              }                             
                              break;

                            case midi::NoteOff: // Detect note off
                              if ( MIDI.getChannel() < 16 ) 
                              {
                                    MIDI.send(MIDI.getType(),
                                    MIDI.getData1(),
                                    MIDI.getData2(),
                                    MIDI.getChannel());                                                      
                              }                             
                              break;                       
                                                        
                            case midi::Clock: // Detect clock

                             // Set downbeat only - no start
                              if ( downBeatOnly == 1 ) {
                                MIDITicksCounter = 0;
                                counterDislay = 1.00;
                                led = 1; bitSet(led, 0); updateShiftRegister(); // LED 1
                                downBeatOnly = 0;
                              }
    
                              // Send a start byte first when downbeat is being set
                              if ( downBeatAndStart == 1 ) {
                                MIDITicksCounter = 0;
                                counterDislay = 1.00;
                                led = 1; bitSet(led, 0); updateShiftRegister(); // LED 1
                                MIDI.sendStart();
                                delay(4);
                                downBeatAndStart = 0;
                              }
                            
                              // Send a start byte followed by a clock byte at downbeat after the Armed Start button has been pressed
                              if ( MIDITicksCounter == 0 && SendStart == 1 ) {
                                MIDI.sendStart();
                                delay(4);
                                SendStart = 0;
                              }
            
                              // Skip a clock byte after the slow down button has been pressed  
                              if (slowDownButton.pressed()) {
                                // Do nothing but disable SendClock and reduce tick count by one
                                SendClock = 0;
                                MIDITicksCounter--;
                              }
            
                              // Insert a clock byte after the speed up button has been pressed                    
                              if (speedUpButton.pressed()) {
                                delay(4);
                                MIDI.sendClock(); // Inserting a a clockbyte 0(= speeding up)
                                delay(4);
                                MIDITicksCounter++;
                              }
                                                                              
                              if ( SendClock == 0 ) { 
                                // Do nothing but enable SendClock again
                                SendClock = 1;
                              }                  
                              else {
                                MIDI.sendClock();
                              }
                                                                   
                                              // Drive LEDs
                                              if ( MIDITicksCounter == 0 )  { 
                                                led = 0; bitSet(led, 7); updateShiftRegister(); // LED 1
                                                counterDislay = 1.00;
                                                
                                                digitalWrite(pin_a, LOW); // "1"
                                                digitalWrite(pin_b, LOW);
                                                digitalWrite(pin_c, LOW);
                                                
                                                CheckBrightness();
                                                }
                            
                                                    if ( MIDITicksCounter == 12 )  { 
                                                    led = 0; bitSet(led, 6); updateShiftRegister(); // LED 2

                                                      if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, HIGH); // "2"
                                                        digitalWrite(pin_b, LOW);
                                                        digitalWrite(pin_c, LOW);                                                        
                                                      }

                                                    CheckBrightness();
                                                    }
                                              
                                              if ( MIDITicksCounter == 24 ) { 
                                                led = 0; bitSet(led, 5); updateShiftRegister(); // LED 3
                                                counterDislay = 2.00;
                                                
                                                    if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, LOW); // "3"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, LOW);                                                        
                                                      }
                                                    else {
                                                        digitalWrite(pin_a, HIGH); // "2"
                                                        digitalWrite(pin_b, LOW);
                                                        digitalWrite(pin_c, LOW);
                                                      }
                                                
                                                CheckBrightness();
                                                }
                                
                                                    if ( MIDITicksCounter == 36 )  { 
                                                    led = 0; bitSet(led, 4); updateShiftRegister(); // LED 4

                                                      if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, HIGH); // "4"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, LOW);                                                        
                                                      }

                                                    CheckBrightness();
                                                    }
                                              
                                              if ( MIDITicksCounter == 48 ) { 
                                                led = 0; bitSet(led, 3); updateShiftRegister(); // LED 5
                                                counterDislay = 3.00;

                                                    if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, LOW); // "5"
                                                        digitalWrite(pin_b, LOW);
                                                        digitalWrite(pin_c, HIGH);                                                        
                                                      }
                                                    else {
                                                        digitalWrite(pin_a, LOW); // "3"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, LOW);
                                                      }
                                                
                                                CheckBrightness();
                                                }
                            
                                                  if ( MIDITicksCounter == 60 )  { 
                                                    led = 0; bitSet(led, 2); updateShiftRegister(); // LED 6

                                                      if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, HIGH); // "6"
                                                        digitalWrite(pin_b, LOW);
                                                        digitalWrite(pin_c, HIGH);                                                        
                                                      }

                                                    CheckBrightness();
                                                    }
                                              
                                              if ( MIDITicksCounter == 72 ) { 
                                                led = 0; bitSet(led, 1); updateShiftRegister(); // LED 7 
                                                counterDislay = 4.00;

                                                    if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, LOW); // "7"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, HIGH);                                                        
                                                      }
                                                    else {
                                                        digitalWrite(pin_a, HIGH); // "4"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, LOW);
                                                      }
                                                
                                                CheckBrightness();
                                                }
                            
                                                    if ( MIDITicksCounter == 84 )  { 
                                                    led = 1; bitSet(led, 0); updateShiftRegister(); // LED 8

                                                      if ( NixieMode == 1 ) {
                                                        digitalWrite(pin_a, HIGH); // "8"
                                                        digitalWrite(pin_b, HIGH);
                                                        digitalWrite(pin_c, HIGH);                                                        
                                                      }

                                                    CheckBrightness();
                                                    }
                                                      
                                                      
                                                      if ( MIDITicksCounter >= 0  && MIDITicksCounter <=  3 ) { if (randomLEDflag == 0){ pixels.clear(); } LEDcolor = 0; LEDnumber = 0; Ringfunction_A(); LEDcolorRND = random(21); } 
                                                      if ( MIDITicksCounter >= 4  && MIDITicksCounter <=  7 ) { Ringfunction_B(); LEDnumber = 1; }    
                                                      if ( MIDITicksCounter >= 8  && MIDITicksCounter <= 11 ) { Ringfunction_A(); LEDnumber = 2; } 
                                                      if ( MIDITicksCounter >= 12 && MIDITicksCounter <= 15 ) { Ringfunction_B(); LEDnumber = 3; }
                                                      if ( MIDITicksCounter >= 16 && MIDITicksCounter <= 19 ) { Ringfunction_A(); LEDnumber = 4; }
                                                      if ( MIDITicksCounter >= 20 && MIDITicksCounter <= 23 ) { Ringfunction_B(); LEDnumber = 5; }
                                                      
                                                      if ( MIDITicksCounter >= 24 && MIDITicksCounter <= 27 ) { if (randomLEDflag == 0){ pixels.clear(); } LEDcolor = 1; LEDnumber = 6; Ringfunction_A();  LEDcolorRND = random(21); }
                                                      if ( MIDITicksCounter >= 28 && MIDITicksCounter <= 31 ) { Ringfunction_B(); LEDnumber = 7; }
                                                      if ( MIDITicksCounter >= 32 && MIDITicksCounter <= 35 ) { Ringfunction_A(); LEDnumber = 8; }
                                                      if ( MIDITicksCounter >= 36 && MIDITicksCounter <= 39 ) { Ringfunction_B(); LEDnumber = 9; }
                                                      if ( MIDITicksCounter >= 40 && MIDITicksCounter <= 43 ) { Ringfunction_A(); LEDnumber = 10; }
                                                      if ( MIDITicksCounter >= 44 && MIDITicksCounter <= 47 ) { Ringfunction_B(); LEDnumber = 11; }
                                                      
                                                      if ( MIDITicksCounter >= 48 && MIDITicksCounter <= 51 ) { if (randomLEDflag == 0){ pixels.clear(); } LEDcolor = 2; LEDnumber = 12; Ringfunction_A(); LEDcolorRND = random(21); }
                                                      if ( MIDITicksCounter >= 52 && MIDITicksCounter <= 55 ) { Ringfunction_B(); LEDnumber = 13; }
                                                      if ( MIDITicksCounter >= 56 && MIDITicksCounter <= 59 ) { Ringfunction_A(); LEDnumber = 14; }
                                                      if ( MIDITicksCounter >= 60 && MIDITicksCounter <= 63 ) { Ringfunction_B(); LEDnumber = 15; }
                                                      if ( MIDITicksCounter >= 64 && MIDITicksCounter <= 67 ) { Ringfunction_A(); LEDnumber = 16; }
                                                      if ( MIDITicksCounter >= 68 && MIDITicksCounter <= 71 ) { Ringfunction_B(); LEDnumber = 17; }
                                                      
                                                      if ( MIDITicksCounter >= 72 && MIDITicksCounter <= 75 ) { if (randomLEDflag == 0){ pixels.clear(); } LEDcolor = 3; LEDnumber = 18; Ringfunction_A(); LEDcolorRND = random(21); }
                                                      if ( MIDITicksCounter >= 76 && MIDITicksCounter <= 79 ) { Ringfunction_B(); LEDnumber = 19; }
                                                      if ( MIDITicksCounter >= 80 && MIDITicksCounter <= 83 ) { Ringfunction_A(); LEDnumber = 20; }
                                                      if ( MIDITicksCounter >= 84 && MIDITicksCounter <= 87 ) { Ringfunction_B(); LEDnumber = 21; }
                                                      if ( MIDITicksCounter >= 88 && MIDITicksCounter <= 91 ) { Ringfunction_A(); LEDnumber = 22; }
                                                      if ( MIDITicksCounter >= 92 && MIDITicksCounter <= 95 ) { Ringfunction_B(); LEDnumber = 23; }     

                         
                                              MIDITicksCounter++;
                                              
                                              if ( MIDITicksCounter >= 96 ) {            
                                               
                                                MIDITicksCounter = 0; 
                                                digitalWrite(12, LOW);                 
                                                }
                            
                                
                                break;
                            // See the online reference for other message types
                            default:
                                break;
                        }
                    












                





        }

}



void updateShiftRegister()
{
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, led);
    digitalWrite(latchPin, HIGH);
}

int Ringfunction_A(void)
{   
  if (randomLEDflag == 1){
    LEDcolor = LEDcolorRND;
    LEDnumber = random(25);
    } 
      
   pixels.setPixelColor(LEDnumber, pixels.Color(colorR[LEDcolor],colorG[LEDcolor],colorB[LEDcolor]));
   pixels.setBrightness(LEDbrightness);
   pixels.show();
}

int Ringfunction_B(void)
{   
  if (randomLEDflag == 1){
    LEDcolor = LEDcolorRND;
    LEDnumber = random(25);
    } 

   pixels.setPixelColor(LEDnumber, pixels.Color(colorR[LEDcolor],colorG[LEDcolor],colorB[LEDcolor]));
   pixels.setBrightness(LEDbrightness);
   pixels.show();
}

int CheckBrightness(void)
{   
  LEDbrightness = analogRead(A3); 
  LEDbrightness = map(LEDbrightness, 21, 1015, 0, 150);
  if ( LEDbrightness < 2 ) { LEDbrightness = 0; } // Making sure LEDs are off below threshold
  
}
