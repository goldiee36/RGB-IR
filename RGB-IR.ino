#include <IRremote.h>
#include <EEPROM.h>
//PIN connections:
//RGB LED strip: red:D9, green:D10, blue:D6
//IR Data: D7
//POWER IN (5V) and GRNs needed for the arduino and the IR sensor, MOSFET pack need GND, LED strip needs 12V on the common
//PIR sensor data: D4
//extra light PWM (bathroom mirror led or extractor hood light): D5

byte RECV_PIN = 7;
byte redPin = 9;
byte greenPin = 10;
byte bluePin = 6;
byte pirPin = 4;
byte extraPin = 5;

byte redCur = 0;
byte greenCur = 0;
byte blueCur = 0;
byte extraCur = 0;
byte redSave = 100;
byte greenSave = 100;
byte blueSave = 100;
byte extraSave = 100;
byte buttonStep = 7 ;
boolean needDelay = true; //disable the loop delay if we do a color transition anyway
byte redRef; //used for the ligth ammount change, like reference values of the original colors
byte greenRef;
byte blueRef;
byte extraRef;
byte refBrightness;
byte curBrightness;
float newBrightnessPercent;
float extraNewBrightnessPercent;
boolean autoCtrl = true;
boolean autoOff = false;
unsigned long autoSwitchoffTimer = 90; //seconds
unsigned long prevMillis;

IRrecv irrecv(RECV_PIN);

decode_results results;
int lastIR;
boolean normalmode;
boolean validIR;
byte repeat_counter;

void setup()
{
  pinMode(redPin, OUTPUT); pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); pinMode(extraPin, OUTPUT);
  setColourRgb(0,0,0,0);
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  normalmode = false;
  validIR = false;
  repeat_counter = 0;
  pinMode(pirPin, INPUT);
}

void loop() {
  if (irrecv.decode(&results)) {
    irrecv.resume();
    Serial.println(results.value, HEX);
    if (results.value==0xFFFFFFFF) {
      //Serial.println("                      majdnem   ismetles");
      if (repeat_counter > 1 && lastIR != 0) {
        switch(lastIR) {
          case 0xFFC13E:
            Serial.println(" ON REP");
            setColourRgb(redSave,greenSave,blueSave,extraSave);
            break;
            
          case 0xFF41BE:
            Serial.println(" OFF");
            if (redCur != 0 || greenCur != 0 || blueCur != 0 || extraCur != 0) {
              redSave = redCur;
              greenSave = greenCur;
              blueSave = blueCur;
              extraSave = extraCur;
              setColourRgb(0,0,0,0);
            }
            break;
            
          case 0xFF817E:
            Serial.println(" DOWN");
            refBrightness=max(redRef,max(greenRef,blueRef));
            curBrightness=max(redCur,max(greenCur,blueCur));
            newBrightnessPercent=(curBrightness - 0.03 * 255) / refBrightness; //0.03 means 3% step in brigthness
            extraNewBrightnessPercent=(extraCur - 0.03 * 255) / extraRef;
            setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 140);
            break;
            
          case 0xFF01FE:
            Serial.println(" UP");
            refBrightness=max(redRef,max(greenRef,blueRef));
            curBrightness=max(redCur,max(greenCur,blueCur));
            newBrightnessPercent=(curBrightness + 0.03 * 255) / refBrightness;
            extraNewBrightnessPercent=(extraCur + 0.03 * 255) / extraRef;
            if (redRef*newBrightnessPercent <= 255 && greenRef*newBrightnessPercent <= 255 && blueRef*newBrightnessPercent <= 255) {
              setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 140);
            }
            break;
            
          case 0xFFE11E:
            Serial.println(" EXTRA+");
            setColourRgb(redCur,greenCur,blueCur,extraCur+buttonStep, true, 140);
            break;
            
          case 0xFF619E:
            Serial.println(" BLUE+");
            setColourRgb(redCur,greenCur,blueCur+buttonStep,extraCur, true, 140);
            break;
            
          case 0xFFA15E:
            Serial.println(" GREEN+");
            setColourRgb(redCur,greenCur+buttonStep,blueCur,extraCur, true, 140);
            break;
            
          case 0xFF21DE:
            Serial.println(" RED+");
            setColourRgb(redCur+buttonStep,greenCur,blueCur,extraCur, true, 140);
            break;
            
          case 0xFFD12E:
            Serial.println(" EXTRA-");
            setColourRgb(redCur,greenCur,blueCur,extraCur-buttonStep, true, 140);
            break;
            
          case 0xFF51AE:
            Serial.println(" BLUE-");
            setColourRgb(redCur,greenCur,blueCur-buttonStep,extraCur, true, 140);
            break;
            
          case 0xFF916E:
            Serial.println(" GREEN-");
            setColourRgb(redCur,greenCur-buttonStep,blueCur,extraCur, true, 140);
            break;
            
          case 0xFF11EE:
            Serial.println(" RED-");
            setColourRgb(redCur-buttonStep,greenCur,blueCur,extraCur, true, 140);
            break;
            
          case 0xFF31CE:
            Serial.println(" MEM1");
            EEPROM.write(3, redCur); EEPROM.write(4, greenCur); EEPROM.write(5, blueCur); EEPROM.write(6, extraCur);
            break;
            
          case 0xFFB14E:
            Serial.println(" MEM2");
            EEPROM.write(7, redCur); EEPROM.write(8, greenCur); EEPROM.write(9, blueCur); EEPROM.write(10, extraCur);
            break;
            
          case 0xFF718E:
            Serial.println(" MEM3");
            EEPROM.write(11, redCur); EEPROM.write(12, greenCur); EEPROM.write(13, blueCur); EEPROM.write(14, extraCur);
            break;
            
          case 0xFF09F6:
            Serial.println(" MEM4");
            EEPROM.write(15, redCur); EEPROM.write(16, greenCur); EEPROM.write(17, blueCur); EEPROM.write(18, extraCur);
            break;
            
          case 0xFF8976:
            Serial.println(" MEM5");
            EEPROM.write(19, redCur); EEPROM.write(20, greenCur); EEPROM.write(21, blueCur); EEPROM.write(22, extraCur);
            break;
            
          case 0xFF49B6:
            Serial.println(" MEM6");
            EEPROM.write(23, redCur); EEPROM.write(24, greenCur); EEPROM.write(25, blueCur); EEPROM.write(26, extraCur);
            break;
            
          case 0xFF29D6:
            Serial.println(" MEM7");
            EEPROM.write(27, redCur); EEPROM.write(28, greenCur); EEPROM.write(29, blueCur); EEPROM.write(30, extraCur);
            break;
            
          case 0xFFA956:
            Serial.println(" MEM8");
            EEPROM.write(31, redCur); EEPROM.write(32, greenCur); EEPROM.write(33, blueCur); EEPROM.write(34, extraCur);
            break;
            
          case 0xFF6996:
            Serial.println(" MEM9");
            EEPROM.write(35, redCur); EEPROM.write(36, greenCur); EEPROM.write(37, blueCur); EEPROM.write(38, extraCur);
            break;
        }
        //Serial.print(lastIR, HEX);
        //Serial.println(" - ISMETLES");
        normalmode = false;
      }
      else {
        normalmode = true;
      }
      if (repeat_counter < 10) { repeat_counter++; }
    }
    else {
      lastIR=results.value;
      normalmode = true;
    }
  }
  else {
    if (normalmode==true && lastIR != 0) {
      switch(lastIR) {
          case 0xFFC13E:
            Serial.println(" ON");
            setColourRgb(redSave,greenSave,blueSave,extraSave);
            break;
            
          case 0xFF41BE:
            Serial.println(" OFF");
            if (redCur != 0 || greenCur != 0 || blueCur != 0 || extraCur != 0) {
              redSave = redCur;
              greenSave = greenCur;
              blueSave = blueCur;
              setColourRgb(0,0,0,0);
            }
            break;
            
          case 0xFF817E:
            Serial.println(" DOWN");
            refBrightness=max(redRef,max(greenRef,blueRef));
            curBrightness=max(redCur,max(greenCur,blueCur));
            newBrightnessPercent=(curBrightness - 0.03 * 255) / refBrightness;
            extraNewBrightnessPercent=(extraCur - 0.03 * 255) / extraRef;
            setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 300);
            break;
            
          case 0xFF01FE:
            Serial.println(" UP");
            refBrightness=max(redRef,max(greenRef,blueRef));
            curBrightness=max(redCur,max(greenCur,blueCur));
            newBrightnessPercent=(curBrightness + 0.03 * 255) / refBrightness;
            extraNewBrightnessPercent=(extraCur + 0.03 * 255) / extraRef;
            if (redRef*newBrightnessPercent <= 255 && greenRef*newBrightnessPercent <= 255 && blueRef*newBrightnessPercent <= 255) {
              setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 300);
            }
            break;
            
          case 0xFFE11E:
            Serial.println(" WHITE");
            setColourRgb(redCur,greenCur,blueCur,extraCur+buttonStep, true, 300);
            break;
            
          case 0xFF619E:
            Serial.println(" BLUE+");
            setColourRgb(redCur,greenCur,blueCur+buttonStep,extraCur, true, 300);
            break;
            
          case 0xFFA15E:
            Serial.println(" GREEN+");
            setColourRgb(redCur,greenCur+buttonStep,blueCur,extraCur, true, 300);
            break;
            
          case 0xFF21DE:
            Serial.println(" RED+");
            setColourRgb(redCur+buttonStep,greenCur,blueCur,extraCur, true, 300);
            break;
            
          case 0xFFD12E:
            Serial.println(" EXTRA-");
            setColourRgb(redCur,greenCur,blueCur,extraCur-buttonStep, true, 300);
            break;
            
          case 0xFF51AE:
            Serial.println(" BLUE-");
            setColourRgb(redCur,greenCur,blueCur-buttonStep,extraCur, true, 300);
            break;
            
          case 0xFF916E:
            Serial.println(" GREEN-");
            setColourRgb(redCur,greenCur-buttonStep,blueCur,extraCur, true, 300);
            break;
            
          case 0xFF11EE:
            Serial.println(" RED-");
            setColourRgb(redCur-buttonStep,greenCur,blueCur,extraCur, true, 300);
            break;
            
          case 0xFF31CE:
            Serial.println(" MEM1");
            setColourRgb(EEPROM.read(3),EEPROM.read(4),EEPROM.read(5),EEPROM.read(6));
            break;
            
          case 0xFFB14E:
            Serial.println(" MEM2");
            setColourRgb(EEPROM.read(7),EEPROM.read(8),EEPROM.read(9),EEPROM.read(10));
            break;
            
          case 0xFF718E:
            Serial.println(" MEM3");
            setColourRgb(EEPROM.read(11),EEPROM.read(12),EEPROM.read(13),EEPROM.read(14));
            break;
            
          case 0xFF09F6:
            Serial.println(" MEM4");
            setColourRgb(EEPROM.read(15),EEPROM.read(16),EEPROM.read(17),EEPROM.read(18));
            break;
            
          case 0xFF8976:
            Serial.println(" MEM5");
            setColourRgb(EEPROM.read(19),EEPROM.read(20),EEPROM.read(21),EEPROM.read(22));
            break;
            
          case 0xFF49B6:
            Serial.println(" MEM6");
            setColourRgb(EEPROM.read(23),EEPROM.read(24),EEPROM.read(25),EEPROM.read(26));
            break;
            
          case 0xFF29D6:
            Serial.println(" MEM7");
            setColourRgb(EEPROM.read(27),EEPROM.read(28),EEPROM.read(29),EEPROM.read(30));
            break;
            
          case 0xFFA956:
            Serial.println(" MEM8");
            setColourRgb(EEPROM.read(31),EEPROM.read(32),EEPROM.read(33),EEPROM.read(34));
            break;
            
          case 0xFF6996:
            Serial.println(" MEM9");
            setColourRgb(EEPROM.read(35),EEPROM.read(36),EEPROM.read(37),EEPROM.read(38));
            break;
        }
      //Serial.print(lastIR, HEX);
      //Serial.println(" - normal");
      normalmode = false;
    }
    repeat_counter = 0;
    lastIR=0;
  }  
  if (colorWheel == true) {
    
  }
  //AUTOMATED CODE BASED ON PIR SENSOR analogRead(0)
  if (autoCtrl == true) { 
    if (digitalRead(pirPin) == HIGH && (analogRead(0) < 250 || autoOff == true) && (millis() - prevMillis) > 1500) {
      if (autoOff == false) { //we dont need to set the MEM1 color again, it would take one sec so the normal IR detection would be sluggish if movement is detected
        Serial.println("AUTO turn on MEM1");
        Serial.println(analogRead(0));
        setColourRgb(EEPROM.read(3),EEPROM.read(4),EEPROM.read(5),EEPROM.read(6)); //automated mode uses the MEM1 settings
        autoCtrl = true; //set the automated mode back to true because setColourRgb swiches off by default
        autoOff = true; //set the automated off mode back to true because setColourRgb swiches off by default
      }
      Serial.println("renew timer");
      prevMillis = millis(); //last movement's timestamp
    }
    if (autoOff == true && (millis() - prevMillis) > (autoSwitchoffTimer * 1000)) { // < 0 overflow detection, in case of OF it turns off also
      Serial.println("AUTO turn off lights");
      setColourRgb(0,0,0,0); //auto off mode is set by the setColourRgb(0,0,0)
      prevMillis = millis(); //last movement's timestamp
    }
  }
  if (needDelay) {
    delay(150); //below 150 it could be unstable to detect the long presses
  }
  needDelay=true;
}


void setColourRgb(int redSet1, int greenSet1, int blueSet1, int extraSet1) {
  setColourRgb(redSet1, greenSet1, blueSet1, extraSet1, true, 1000);
}

void setColourRgb(int redSet, int greenSet, int blueSet, int extraSet, boolean changeRef, int tranSpeed) {
  if (redSet < 0) { redSet = 0; }
  if (greenSet < 0) { greenSet = 0; }
  if (blueSet < 0) { blueSet = 0; }
  if (extraSet < 0) { extraSet = 0; }
  if (redSet > 255) { redSet = 255; }
  if (greenSet > 255) { greenSet = 255; }
  if (blueSet > 255) { blueSet = 255; }
  if (extraSet > 255) { extraSet = 255; }
  float redStep = (redSet - redCur)/float(tranSpeed);
  float greenStep = (greenSet - greenCur)/float(tranSpeed);
  float blueStep = (blueSet - blueCur)/float(tranSpeed);
  float extraStep = (extraSet - extraCur)/float(tranSpeed);
  for (int i = 1; i <= tranSpeed; i++) {
    analogWrite(redPin, redCur+0.5+redStep*i);
    analogWrite(greenPin, greenCur+0.5+greenStep*i);
    analogWrite(bluePin, blueCur+0.5+blueStep*i);
    analogWrite(extraPin, extraCur+0.5+extraStep*i);
    delay(1);
  }
  redCur = redSet;
  greenCur = greenSet;
  blueCur = blueSet;
  extraCur = extraSet;
  needDelay = false;
  if (changeRef) {
    redRef = redSet;
    greenRef = greenSet;
    blueRef = blueSet;
    extraRef = extraSet;
  }
  if (redSet == 0 && greenSet == 0 && blueSet == 0 && extraSet == 0) {//AUTO mode ON if all colors are 0 (like OFF button, or just manual zero light)
    autoCtrl = true;
    autoOff = false;
  }
  else {
    autoCtrl = false;
    autoOff = false;
  }
} 
