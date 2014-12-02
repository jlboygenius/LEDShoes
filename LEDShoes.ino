/* ======================
LED Shoes
LED strips wrapped around shoes, controlled by two ADXL345 accelerometers in the shoes

based on code from: https://github.com/jrowberg/i2cdevlib for the adxl345

 */

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#include "Wire.h"

// I2Cdev and ADXL345 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "ADXL345.h"
#include "LPD8806.h"
#include <SPI.h>

// class default I2C address is 0x53
// specific I2C addresses may be passed as a parameter here
// ALT low = 0x53 (default for SparkFun 6DOF board)
// ALT high = 0x1D
ADXL345 accelL(ADXL345_ADDRESS_ALT_HIGH);
ADXL345 accelR(ADXL345_ADDRESS_ALT_LOW);

int16_t ax, ay, az;
int16_t bx, by, bz;

#define LED_PIN 13 // (Arduino is 13, Teensy is 6)
bool blinkState = false;

int LeftStatus = 0;// 1 - tap, 2-doubleTap,3-inactive,0-active
int RightStatus = 0; // 1 - tap, 2-doubleTap,3-inactive,0-active
int CurrentState = 0;


//pins for LEDs
int clockPin = 8; // for the LED strips
int dataPinL  = 10; // data pins for the LED strips
int dataPinR  = 9; // data pins for the LED strips
const int nLEDs = 38; // number of LED's per strip
const int BRIGHTNESS = 25;
const int STEPWAITLENGTH = 45; // number of times through the loop to keep the lights lit for a step
const int CHASESPACE=5; // number of spaces between dots in the chase around the loop
int SPINSPEED=20; // number of times through the loop before moving turning it
// rotary encoder pins
const int ROTARYSWITCH = A6;
const int ROTARYBLUE = 3;
const int ROTARYRED = 6;
const int ROTARYGREEN = 5;

uint16_t LeftRainbowWheel = 0;
uint16_t RightRainbowWheel = 0;


int encoderPin1 = 2;
int encoderPin2 = 7;
int encoderPinC = 4;

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;

int rotaryR=0;
int rotaryB=0;
int rotaryG=0;

int LChaseSpace = 5;
int RChaseSpace = 5;
int SpinSpace =5;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 stripL = LPD8806(nLEDs, dataPinL, clockPin);
LPD8806 stripR = LPD8806(nLEDs, dataPinR, clockPin);


  uint8_t lightPosR[nLEDs];
  uint8_t lightPosG[nLEDs];
  uint8_t lightPosB[nLEDs];  
uint32_t LightPosRight[nLEDs];
uint32_t LightPosLeft[nLEDs];

int LeftWaitLoops=0;
int RightWaitLoops =0;



void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();
 randomSeed(analogRead(3));
    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(38400);

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelL.initialize();
    accelR.initialize();    


// LED strip
  pinMode(clockPin, OUTPUT);
  pinMode(dataPinL, OUTPUT);
  pinMode(dataPinR, OUTPUT);
  pinMode(encoderPinC,OUTPUT);  
  
  stripL.begin();
  stripL.show();
  stripR.begin();
  stripR.show();


// spiral lights
  for (int x=0;x<nLEDs;x++){
   lightPosR[x]=0; 
   lightPosG[x]=0; 
   lightPosB[x]=0;    
  }
  setDotSpacing(lightPosR,lightPosG,lightPosB,5);


   


  setupAxcells();



  //rotary encoder
  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);
  pinMode(ROTARYGREEN,OUTPUT);  
  pinMode(ROTARYRED,OUTPUT);  
  pinMode(ROTARYBLUE,OUTPUT);      

  pinMode(ROTARYSWITCH, INPUT);
  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on
  digitalWrite(encoderPinC,LOW);
  digitalWrite(ROTARYSWITCH, HIGH); //turn pullup resistor on
//call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);
 




    // configure LED for output
  pinMode(LED_PIN, OUTPUT);
}

void loop() {


   
   if(encoderValue>=0 &&encoderValue<10){
     // off,random color on step
     setRotaryColor(0,255,255);
     if(accelL.getIntSingleTapSource()){
        Serial.println("  Left tap");
        setStepColor(stripL, RandomGoodColor());
//        colorWipe3(stripL,random(0, 255),random(0, 255),random(0, 255),BRIGHTNESS);      
         LeftWaitLoops = STEPWAITLENGTH;
         stripL.show();

      }
      
      if(accelR.getIntSingleTapSource()){
        Serial.println("  Right tap");
        setStepColor(stripR, RandomGoodColor());
//        colorWipe3(stripR,random(0, 255),random(0, 255),random(0, 255),BRIGHTNESS);      
        RightWaitLoops  =STEPWAITLENGTH;
        stripR.show();
      }
      if(LeftWaitLoops>0) LeftWaitLoops--;
      if(RightWaitLoops>0) RightWaitLoops--;
 
    if(LeftWaitLoops==0){   
        colorWipe3(stripL,0,0,0,BRIGHTNESS);      
    }
    if(RightWaitLoops==0){
        colorWipe3(stripR,0,0,0,BRIGHTNESS);      
    }
 
   }else if(encoderValue>=10 &&encoderValue<20){
     // chasing white, random color on step
          setRotaryColor(0,128,255);
          computeSpinSpeed();
       if(accelL.getIntSingleTapSource()){
        Serial.println("  Left tap");
          setStepColor(stripL, RandomGoodColor());
//          colorWipe3(stripL,random(0, 255),random(0, 255),random(0, 255),BRIGHTNESS);      
        
        LeftWaitLoops = STEPWAITLENGTH;
        stripL.show();
      }
      
      if(accelR.getIntSingleTapSource()){
        Serial.println("  Right tap");
        setStepColor(stripR, RandomGoodColor());        
//        colorWipe3(stripR,random(0, 255),random(0, 255),random(0, 255),BRIGHTNESS);      
        RightWaitLoops  =STEPWAITLENGTH;
        stripR.show();
      }
      if(LeftWaitLoops>0) LeftWaitLoops--;
      if(RightWaitLoops>0) RightWaitLoops--;
 
 
      if(SpinSpace<=0){
           if(LeftWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
               if(LChaseSpace==0){
                 AddToChaseL(WhiteColor());
                 LChaseSpace=CHASESPACE;
               }else{
                 AddToChaseL(OffColor());                              
                 LChaseSpace--;
               }
          }     
          if(RightWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
             if(RChaseSpace==0){
               AddToChaseR(WhiteColor());               
               RChaseSpace=CHASESPACE;
             }else{
               AddToChaseR(OffColor());                              
               RChaseSpace--;
             }
          }  
          SpinSpace = SPINSPEED;
       } else{
         SpinSpace--;
         
       }
 
     
     
   }else if(encoderValue>=20 &&encoderValue<30){
       setRotaryColor(0,255,0);
       computeSpinSpeed();

  // chasing Random, random color on step
       if(accelL.getIntSingleTapSource()){
        Serial.println("  Left tap");
        setStepColor(stripL, RandomGoodColor());
        LeftWaitLoops = STEPWAITLENGTH;
        stripL.show();
      }
      
      if(accelR.getIntSingleTapSource()){
        Serial.println("  Right tap");
        setStepColor(stripR, RandomGoodColor());
        RightWaitLoops  =STEPWAITLENGTH;
        stripR.show();
      }
      if(LeftWaitLoops>0) LeftWaitLoops--;
      if(RightWaitLoops>0) RightWaitLoops--;
 
 
      if(SpinSpace<=0){
           if(LeftWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
                 if(LChaseSpace==0){
                          AddToChaseL(RandomGoodColor());
                   LChaseSpace=CHASESPACE;
                 }else{
                   AddToChaseL(OffColor());
                   LChaseSpace--;
                 }
               stripL.show();
          }     
          if(RightWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
             if(RChaseSpace==0){
                AddToChaseR(RandomGoodColor());
                RChaseSpace=CHASESPACE;
             }else{
               AddToChaseR(OffColor());               
               RChaseSpace--;
             }
             stripR.show();
          }  
          SpinSpace = SPINSPEED;
       } else{
         SpinSpace--;
         
       }
     
   }else if(encoderValue>=30 &&encoderValue<40){  
     //fading rainbow, random color on step
            setRotaryColor(255,0,128);
       computeSpinSpeed();
       SPINSPEED = SPINSPEED/5;
       if(accelL.getIntSingleTapSource()){
        Serial.println("  Left tap");
        setStepColor(stripL, RandomGoodColor());
        LeftWaitLoops = STEPWAITLENGTH;
        stripL.show();
      }
      
      if(accelR.getIntSingleTapSource()){
        Serial.println("  Right tap");
        setStepColor(stripR, RandomGoodColor());
        RightWaitLoops  =STEPWAITLENGTH;
        stripR.show();
      }
      if(LeftWaitLoops>0) LeftWaitLoops--;
      if(RightWaitLoops>0) RightWaitLoops--;
 
     
     if(SpinSpace<=0){
         if(LeftWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
                 AddToChaseL(Wheel(LeftRainbowWheel));
                 LeftRainbowWheel++;
                 if(LeftRainbowWheel==384) LeftRainbowWheel =0;             
          }     
          if(RightWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
            AddToChaseR(Wheel(RightRainbowWheel));
            RightRainbowWheel++;
            if(RightRainbowWheel==384) RightRainbowWheel =0;  
          }  
         SpinSpace = SPINSPEED;
       } else{
         SpinSpace--;
       }
     
     
            
   }else if(encoderValue>=40 &&encoderValue<50){  
//chasing rainbow, random color on step
            setRotaryColor(255,128,128);
       computeSpinSpeed();
       SPINSPEED = SPINSPEED/5;
       if(accelL.getIntSingleTapSource()){
        Serial.println("  Left tap");
        setStepColor(stripL, RandomGoodColor());
        LeftWaitLoops = STEPWAITLENGTH;
        stripL.show();
      }
      
      if(accelR.getIntSingleTapSource()){
        Serial.println("  Right tap");
        setStepColor(stripR, RandomGoodColor());
        RightWaitLoops  =STEPWAITLENGTH;
        stripR.show();
      }
      if(LeftWaitLoops>0) LeftWaitLoops--;
      if(RightWaitLoops>0) RightWaitLoops--;
 
     
     if(SpinSpace<=0){
         if(LeftWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
                 AddToChaseL(Wheel(  ((LeftRainbowWheel * 384 / nLEDs) +LeftRainbowWheel ) % 384)     );
                 LeftRainbowWheel++;
                 if(LeftRainbowWheel==384) LeftRainbowWheel =0;             
          }     
          if(RightWaitLoops==0){
             // we're not waiting on the step anymore, 
             // add a pixel to the chase, 
            AddToChaseR(Wheel(   ((RightRainbowWheel * 384 / nLEDs) + RightRainbowWheel) % 384));
            RightRainbowWheel++;
            if(RightRainbowWheel==384) RightRainbowWheel =0;  
          }  
         SpinSpace = SPINSPEED;
       } else{
         SpinSpace--;
       }
          
   }else if(encoderValue>=50 &&encoderValue<60){
     // headlights! white front, red back
                 setRotaryColor(255,200,200);
        setstripsoff(); 
         headlights(stripL);
         headlights(stripR);  
     }else if(encoderValue>=60 &&encoderValue<70){  
       // light up based on angle, white
          // read raw accel measurements from device
              accelL.getAcceleration(&ax, &ay, &az);
            accelR.getAcceleration(&bx, &by, &bz);    
          
                setstripsoff();
           if(bx<=-100){ //light up  right right toes
               setStripSegment( stripR, WhiteColor() ,14,21);
           }else if (bx>=100){ // light up right heel
               setStripSegment( stripR, WhiteColor() ,34,38);
               setStripSegment( stripR, WhiteColor() ,1,1);                 
           }
           if(ax>=100){ // light up left toes
               setStripSegment( stripL, WhiteColor() ,14,21);
           }else if (ax<=-100){ // light up left heel
               setStripSegment( stripL, WhiteColor() ,34,38);
               setStripSegment( stripL, WhiteColor() ,1,1);                 
           
           }
           
           if(ay<=-100){// light up left inside
              setStripSegment( stripL, WhiteColor() ,6,11);           
           }else if(ay>=100){ //light up left outside

              setStripSegment( stripL, WhiteColor() ,24,30);           
           }
           if(by<=-100){ // light up right inside
              setStripSegment( stripR, WhiteColor() ,24,30);
           }else if(by>=100){// light up right outside
              setStripSegment( stripR, WhiteColor() ,6,11);
           }
           stripL.show();
           stripR.show();
     
   }else if(encoderValue>=70 &&encoderValue<80){
     // light up based on angle, random color
            setRotaryColor(255,20,128);     
        // read raw accel measurements from device
              accelL.getAcceleration(&ax, &ay, &az);
            accelR.getAcceleration(&bx, &by, &bz);    
             
                setstripsoff();
           if(bx<=-100){ //light up  right right toes
               setStripSegment( stripR, RandomGoodColor() ,14,21);
           }else if (bx>=100){ // light up right heel
               setStripSegment( stripR, RandomGoodColor() ,34,38);
               setStripSegment( stripR, RandomGoodColor() ,1,1);                 
           }
           if(ax>=100){ // light up left toes
               setStripSegment( stripL, RandomGoodColor() ,14,21);
           }else if (ax<=-100){ // light up left heel
               setStripSegment( stripL, RandomGoodColor() ,34,38);
               setStripSegment( stripL, RandomGoodColor() ,1,1);                 
           
           }
           
           if(ay<=-100){// light up left inside
              setStripSegment( stripL, RandomGoodColor() ,6,11);           
           }else if(ay>=100){ //light up left outside

              setStripSegment( stripL, RandomGoodColor() ,24,30);           
           }
           if(by<=-100){ // light up right inside
              setStripSegment( stripR, RandomGoodColor() ,24,30);
           }else if(by>=100){// light up  outside
              setStripSegment( stripR, RandomGoodColor() ,6,11);
           }
           stripL.show();
           stripR.show();
        
   }else if(encoderValue>=80 &&encoderValue<90){
                 setRotaryColor(200,200,245);
        colorWipe3(stripL,0,0,0,BRIGHTNESS);
        colorWipe3(stripR,0,0,0,BRIGHTNESS);    
   }
  
  

    
     
        
     if(encoderValue<0) encoderValue=0;
     if(encoderValue>90)encoderValue=90;
   
 
 }
void setstripsoff(){
   for (int i=0; i < nLEDs; i++) {
        stripL.setPixelColor(i, stripL.Color(0,0,0));
        stripR.setPixelColor(i, stripR.Color(0,0,0));        
    }
    
}
 
void setStripSegment(LPD8806 strp,uint32_t newColor,int start,int segend){
  for(int i=start;i<=segend;i++){
    strp.setPixelColor(i,newColor);
  }
  
}

void AddToChaseL(uint32_t newColor  ){
    for(int y=nLEDs-1;y>0;y--){
      LightPosLeft[y] = LightPosLeft[y-1];
    }
    LightPosLeft[0] = newColor;
    for(int i=0;i<nLEDs;i++){
        stripL.setPixelColor(i, LightPosLeft[i]);
    }
    stripL.show();
}

void AddToChaseR(uint32_t newColor  ){
    for(int y=nLEDs-1;y>0;y--){
      LightPosRight[y] = LightPosRight[y-1];
    }
    LightPosRight[0] = newColor;
    for(int i=0;i<nLEDs;i++){
        stripR.setPixelColor(i, LightPosRight[i]);
    }
    stripR.show();
}

void headlights(LPD8806 strp){
          strp.setPixelColor(14, strp.Color(255,255,255));
          strp.setPixelColor(15, strp.Color(255,255,255));
          strp.setPixelColor(16, strp.Color(255,255,255));
          strp.setPixelColor(17, strp.Color(255,255,255));          
          strp.setPixelColor(18, strp.Color(255,255,255));          
          strp.setPixelColor(19, strp.Color(255,255,255));                    
          strp.setPixelColor(20, strp.Color(255,255,255));                    
          strp.setPixelColor(21, strp.Color(255,255,255));                              

          strp.setPixelColor(34, strp.Color(255,0,0));          
          strp.setPixelColor(35, strp.Color(255,0,0));          
          strp.setPixelColor(36, strp.Color(255,0,0));          
          
          strp.setPixelColor(37, strp.Color(255,0,0));          
          strp.setPixelColor(38, strp.Color(255,0,0));          
          strp.setPixelColor(0, strp.Color(255,0,0));          
          strp.setPixelColor(1, strp.Color(255,0,0));                    
          strp.setPixelColor(2, strp.Color(255,0,0));                    
          strp.show();
} 

void AddToChase(LPD8806 strp,uint8_t newR,uint8_t newG, uint8_t newB  ){
  
    int holderR = newR;
    int holderG = newG;
    int holderB = newB;  
    for(int y=nLEDs-1;y>0;y--){
     lightPosR[y] = lightPosR[y-1];
     lightPosG[y] = lightPosG[y-1];
     lightPosB[y] = lightPosB[y-1];     

    }
    lightPosR[0]   = holderR;
    lightPosG[0]   = holderG;
    lightPosB[0]   = holderB;        
    
    for(int i=0;i<nLEDs;i++){
        strp.setPixelColor(i, strp.Color(lightPosR[i],lightPosR[i],lightPosR[i]));
    }
    strp.show();

}






void chase(LPD8806 strp,int height,int brightness){
  
  
    for(int i=0;i<height;i++){
//       strp.setPixelColor(i, strp.Color(lightPosR[i],lightPosR[i],lightPosR[i]));
       strp.setPixelColor(i, strp.Color(brightness,brightness,brightness));       
    }
  strp.show();
//  Serial.print(lightPos[height-1]);
//  Serial.print(":");
//  Serial.print(lightPos[0] ); 
//  Serial.println();
  int holderR = lightPosR[height-1];
  int holderG = lightPosG[height-1];
  int holderB = lightPosB[height-1];  
    for(int y=height-1;y>0;y--){
     lightPosR[y] = lightPosR[y-1];
     lightPosG[y] = lightPosG[y-1];
     lightPosB[y] = lightPosB[y-1];     
      
    }
        lightPosR[0]   = holderR;
        lightPosG[0]   = holderG;
        lightPosB[0]   = holderB;        
//     for(int z=0;z<height;z++){
//    
//      Serial.print(lightPosR[z]);
//      Serial.print(lightPosG[z]);
//      Serial.print(",");
//     }


}



void setDotSpacing(uint8_t lightsR[],uint8_t lightsG[],uint8_t lightsB[], int spacing){
int spaces = nLEDs   / spacing;
  Serial.print(spaces);
  int y=spaces;
  for(int x=0;x<nLEDs;x++){
    if(y==spaces){
      lightsR[x] = BRIGHTNESS;
      lightsG[x] = BRIGHTNESS;
      lightsB[x] = BRIGHTNESS;      
      
      y=0;
    }else{
      y++;
    }
  }
  
  
}

void setStepColor(LPD8806 strp, uint32_t c){
  for (uint8_t i=0; i < nLEDs; i++) {
      
        strp.setPixelColor(i, c);
   
    }
    strp.show();

}
uint32_t CreateColor( uint32_t r,uint32_t g, uint32_t b){
   return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;  
}
// totally random colors are pretty bad, lets limit it
uint32_t RandomGoodColor(){
 int c = random(1,7);
 uint32_t r,g,b; 

 switch(c){
  case 0: r=255;g=255;b=255;break;
  
  case 1: r=255;g=0;b=0;break;
  case 2:r=0;g=255;b=0;break;
  case 3:r=0;g=0;b=255;  break;

  case 4:r=255;g=0;b=255;  break;  
  case 5:r=0;g=255;b=255;  break;  
  case 6:r=255;g=0;b=255;  break;  
  case 7:r=255;g=255;b=0;  break;  
  
  case 8:r=128;g=0;b=255;  break;  
  case 9:r=0;g=128;b=255;  break;  
  case 10:r=128;g=128;b=255;  break;    
  
  case 11:r=255;g=0;b=128;  break;  
  case 12:r=255;g=128;b=0;  break;  
  case 13:r=255;g=128;b=128;  break;   

  case 14:r=0;g=255;b=128;  break;  
  case 15:r=128;g=255;b=0;  break;  
  case 16:r=128;g=255;b=128;  break;   
  
 }


 return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;  
}

uint32_t OffColor(){
 uint32_t r,g,b; 
 r=0;g=0;b=0;
   return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;  
}

uint32_t WhiteColor(){
 uint32_t r,g,b; 
 r=255;g=255;b=255;
   return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;  
}



void colorWipe3(LPD8806 strp, int r,int g,int b, int brightness) {
    //note: rgb order is really RBG
    int i;
  if (r>256)r=256;
  if (g>256)g=256;
  if (b>256)b=256;
  
  if (r<0)r=0;
  if (g<0)g=0;
  if (b<0)b=0;
  
    int dolight[38];
   
    //first set of lights are green
    for (i=0; i < 38; i++) {
      
        strp.setPixelColor(i, strp.Color(r,g,b));
   
    }
    strp.show();

}
 

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(LPD8806 stripT,uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < stripT.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      stripT.setPixelColor(i, Wheel( ((i * 384 / stripT.numPixels()) + j) % 384) );

    }  
    stripT.show();   // write all the pixels out

    delay(wait);
  }
}

void computeSpinSpeed(){
   int x = encoderValue % 10;
     switch (x){
       case 0:
       case 1:
       case 2: SPINSPEED= 5; break;
       case 3:
       case 4:
       case 5: SPINSPEED= 15; break;   
       case 6:
       case 7:
       case 8:
       case 9: SPINSPEED= 25; break;   
     }
 }

void updateEncoder(){
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  if (encoded>80) encoded =0;

  lastEncoded = encoded; //store this value for next time
}

void setRotaryColor(int R,int G,int B){

  analogWrite( ROTARYRED ,R);
  analogWrite( ROTARYGREEN ,G);  
  analogWrite( ROTARYBLUE ,B);  
}



/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(CreateColor(r,g,b));
}

void setupAxcells(){
  // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelL.testConnection() ? "ADXL345 connection successful" : "ADXL345 connection failed");
        Serial.println(accelR.testConnection() ? "ADXL345B connection successful" : "ADXL345B connection failed");



  //set activity/ inactivity thresholds (0-255)
 accelL.setActivityThreshold(75); //62.5mg per increment
  accelL.setInactivityThreshold(75); //62.5mg per increment
  accelL.setInactivityTime(10); 
 accelR.setActivityThreshold(75); //62.5mg per increment
  accelR.setInactivityThreshold(75); //62.5mg per increment
  accelR.setInactivityTime(10); 

  //look of activity movement on this axes - 1 == on; 0 == off 
  accelL.setActivityXEnabled(1);
  accelL.setActivityYEnabled(1);
  accelL.setActivityZEnabled(1);
  accelR.setActivityXEnabled(1);
  accelR.setActivityYEnabled(1);
  accelR.setActivityZEnabled(1);


  //look of inactivity movement on this axes - 1 == on; 0 == off
  accelL.setInactivityXEnabled(1);
  accelL.setInactivityYEnabled(1);
  accelL.setInactivityZEnabled(1);
  accelR.setInactivityXEnabled(1);
  accelR.setInactivityYEnabled(1);
  accelR.setInactivityZEnabled(1);

//look of tap movement on this axes - 1 == on; 0 == off
  accelL.setTapAxisXEnabled(0);
  accelL.setTapAxisYEnabled(0);
  accelL.setTapAxisZEnabled(1);
  accelR.setTapAxisXEnabled(0);
  accelR.setTapAxisYEnabled(0);
  accelR.setTapAxisZEnabled(1);

  
//set values for what is a tap, and what is a double tap (0-255)
  accelL.setTapThreshold(50); //62.5mg per increment
  accelL.setTapDuration(15); //625μs per increment
  accelL.setDoubleTapLatency(80); //1.25ms per increment
  accelL.setDoubleTapWindow(200); //1.25ms per increment  
  accelR.setTapThreshold(50); //62.5mg per increment
  accelR.setTapDuration(15); //625μs per increment
  accelR.setDoubleTapLatency(80); //1.25ms per increment
  accelR.setDoubleTapWindow(200); //1.25ms per increment  


  //setting all interupts to take place on int pin 1
  //I had issues with int pin 2, was unable to reset it
  accelL.setIntSingleTapEnabled(1);
  accelL.setIntDoubleTapEnabled( 1 );
  accelR.setIntSingleTapEnabled(1);
  accelR.setIntDoubleTapEnabled( 1 );
 
}
