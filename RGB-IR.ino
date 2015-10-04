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
byte redRef; //used for the ligth ammount change, like reference values of the original colors
byte greenRef;
byte blueRef;
byte extraRef;
byte refBrightness;
byte curBrightness;
float newBrightnessPercent;
float extraNewBrightnessPercent;
boolean autoCtrl = true; //automated light switch on and off (when dark enough, no manual control, and there is a movement)
boolean autoOff = false; //automated light switch off needed - this state means that the light are on because of autoctrl switch on
#define autoSwitchoffTimer 120 //seconds
unsigned long lastMovement_millis;

IRrecv irrecv(RECV_PIN);

decode_results results;
int lastIR;
boolean normalmode; //for detecting the short/long presses
boolean validIR;
byte repeat_counter;
#define IRDELAY 150 //means ms - wait between IR detection readouts
unsigned long lastIRreadout_millis = 0;

//color wheel stuff
boolean colorWheel = false; //activate color wheel which power offs after 20 minutes
byte colorWheelR = 128;
byte colorWheelG = 127;
byte colorWheelB = 0;
byte colorWheelWhichColorToDecrease = 0; //0 red, 1 green, 2 blue
byte colorWheelNextColorToDecrease = 0;
unsigned long lastColorWheelChange_millis;
int colorWheelChangeTime = 200; //in milliseconds
byte colorWheelExtraLight = 50;

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
  if ((millis() - lastIRreadout_millis) > IRDELAY) {//the IR detection needs time to evaulate the next button push. in case of long presses the oxFFFFFFFF repeat codes are coming around every 100ms each after an other
                                                                     //If we check the next IR detection too early we may miss the next repeat code
    lastIRreadout_millis = millis();
    if (irrecv.decode(&results)) {
      irrecv.resume();
      Serial.println(results.value, HEX);
      if (results.value==0xFFFFFFFF) {
        Serial.println("                      almost repeat");
        if (repeat_counter > 1 && lastIR != 0) { //one repeat code (FFFFFFFF) is not enough for repeat - it is easy to push the button that long
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
              if (colorWheel == true) {
                colorWheelChangeTime = colorWheelChangeTime - 10 < 0 ? 0 : colorWheelChangeTime - 10 ;
              }
              else {
                refBrightness=max(redRef,max(greenRef,blueRef));
                curBrightness=max(redCur,max(greenCur,blueCur));
                newBrightnessPercent=(curBrightness - 0.05 * 255) / refBrightness; //0.05 means 5% step in brigthness
                extraNewBrightnessPercent=(extraCur - 0.05 * 255) / extraRef;
                setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 140);
              }
              break;
              
            case 0xFF01FE:
              Serial.println(" UP");
              if (colorWheel == true) {
                colorWheelChangeTime = colorWheelChangeTime + 10 > 5000 ? 5000 : colorWheelChangeTime + 10 ;
              }
              else {
                refBrightness=max(redRef,max(greenRef,blueRef));
                curBrightness=max(redCur,max(greenCur,blueCur));
                newBrightnessPercent=(curBrightness + 0.05 * 255) / refBrightness;
                extraNewBrightnessPercent=(extraCur + 0.05 * 255) / extraRef;
                if (redRef*newBrightnessPercent <= 255 && greenRef*newBrightnessPercent <= 255 && blueRef*newBrightnessPercent <= 255) {
                  setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 140);
                }
              }
              break;
              
            case 0xFFE11E:
              Serial.println(" EXTRA+ in repeat");
              if (colorWheel == true) {
                colorWheelExtraLight = colorWheelExtraLight + buttonStep > 255 ? 255 : colorWheelExtraLight + buttonStep ;
              }
              else {
                setColourRgb(redCur,greenCur,blueCur,extraCur+buttonStep, true, 140);
              }
              break;
            
            case 0xFFD12E:
              Serial.println(" EXTRA- in repeat");
              if (colorWheel == true) {
                colorWheelExtraLight = colorWheelExtraLight - buttonStep < 0 ? 0 : colorWheelExtraLight - buttonStep ;
              }
              else {
                setColourRgb(redCur,greenCur,blueCur,extraCur-buttonStep, true, 140);
              }
              break;
              
            case 0xFF619E:
              Serial.println(" BLUE+ in repeat");
              setColourRgb(redCur,greenCur,blueCur+buttonStep,extraCur, true, 140);
              break;
              
            case 0xFFA15E:
              Serial.println(" GREEN+ in repeat");
              setColourRgb(redCur,greenCur+buttonStep,blueCur,extraCur, true, 140);
              break;
              
            case 0xFF21DE:
              Serial.println(" RED+ in repeat");
              setColourRgb(redCur+buttonStep,greenCur,blueCur,extraCur, true, 140);
              break;           
            
            case 0xFF51AE:
              Serial.println(" BLUE- in repeat");
              setColourRgb(redCur,greenCur,blueCur-buttonStep,extraCur, true, 140);
              break;
              
            case 0xFF916E:
              Serial.println(" GREEN- in repeat");
              setColourRgb(redCur,greenCur-buttonStep,blueCur,extraCur, true, 140);
              break;
              
            case 0xFF11EE:
              Serial.println(" RED- in repeat");
              setColourRgb(redCur-buttonStep,greenCur,blueCur,extraCur, true, 140);
              break;
              
            case 0xFF31CE:
              Serial.println(" MEM1 save");
              EEPROM.write(3, redCur); EEPROM.write(4, greenCur); EEPROM.write(5, blueCur); EEPROM.write(6, extraCur);
              break;
              
            case 0xFFB14E:
              Serial.println(" MEM2 save");
              EEPROM.write(7, redCur); EEPROM.write(8, greenCur); EEPROM.write(9, blueCur); EEPROM.write(10, extraCur);
              break;
              
            case 0xFF718E:
              Serial.println(" MEM3 save");
              EEPROM.write(11, redCur); EEPROM.write(12, greenCur); EEPROM.write(13, blueCur); EEPROM.write(14, extraCur);
              break;
              
            case 0xFF09F6:
              Serial.println(" MEM4 save");
              EEPROM.write(15, redCur); EEPROM.write(16, greenCur); EEPROM.write(17, blueCur); EEPROM.write(18, extraCur);
              break;
              
            case 0xFF8976:
              Serial.println(" MEM5 save");
              EEPROM.write(19, redCur); EEPROM.write(20, greenCur); EEPROM.write(21, blueCur); EEPROM.write(22, extraCur);
              break;
              
            case 0xFF49B6:
              Serial.println(" MEM6 save");
              EEPROM.write(23, redCur); EEPROM.write(24, greenCur); EEPROM.write(25, blueCur); EEPROM.write(26, extraCur);
              break;
              
            case 0xFF29D6:
              Serial.println(" MEM7 save");
              EEPROM.write(27, redCur); EEPROM.write(28, greenCur); EEPROM.write(29, blueCur); EEPROM.write(30, extraCur);
              break;
              
            case 0xFFA956:
              Serial.println(" MEM8 save");
              EEPROM.write(31, redCur); EEPROM.write(32, greenCur); EEPROM.write(33, blueCur); EEPROM.write(34, extraCur);
              break;
              
            case 0xFF6996:
              Serial.println(" MEM9 save");
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
              if (colorWheel == true) {
                colorWheelChangeTime = colorWheelChangeTime - 5 < 0 ? 0 : colorWheelChangeTime - 5 ;
              }
              else {
                refBrightness=max(redRef,max(greenRef,blueRef));
                curBrightness=max(redCur,max(greenCur,blueCur));
                newBrightnessPercent=(curBrightness - 0.03 * 255) / refBrightness;
                extraNewBrightnessPercent=(extraCur - 0.03 * 255) / extraRef;
                setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 300);
              }
              break;
              
            case 0xFF01FE:
              Serial.println(" UP");
              if (colorWheel == true) {
                colorWheelChangeTime = colorWheelChangeTime + 5 > 5000 ? 5000 : colorWheelChangeTime + 5 ;
              }
              else {
                refBrightness=max(redRef,max(greenRef,blueRef));
                curBrightness=max(redCur,max(greenCur,blueCur));
                newBrightnessPercent=(curBrightness + 0.03 * 255) / refBrightness;
                extraNewBrightnessPercent=(extraCur + 0.03 * 255) / extraRef;
                if (redRef*newBrightnessPercent <= 255 && greenRef*newBrightnessPercent <= 255 && blueRef*newBrightnessPercent <= 255) {
                  setColourRgb(redRef*newBrightnessPercent+0.5, greenRef*newBrightnessPercent+0.5, blueRef*newBrightnessPercent+0.5, extraRef*extraNewBrightnessPercent+0.5, false, 300);
                }
              }
              break;
              
            case 0xFFE11E:
              Serial.println(" EXTRA+");
              if (colorWheel == true) {
                colorWheelExtraLight = colorWheelExtraLight + buttonStep > 255 ? 255 : colorWheelExtraLight + buttonStep ;
              }
              else {
                setColourRgb(redCur,greenCur,blueCur,extraCur+buttonStep, true, 300);
              }
              break;
            
            case 0xFFD12E:
              Serial.println(" EXTRA-");
              if (colorWheel == true) {
                colorWheelExtraLight = colorWheelExtraLight - buttonStep < 0 ? 0 : colorWheelExtraLight - buttonStep ;
              }
              else {
                setColourRgb(redCur,greenCur,blueCur,extraCur-buttonStep, true, 300);
              }
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
              
            //case 0xFF6996:
            //  Serial.println(" MEM9");
            //  setColourRgb(EEPROM.read(35),EEPROM.read(36),EEPROM.read(37),EEPROM.read(38));
            //  break;
              
            case 0xFF6996:
              Serial.println(" prog1 - color wheel");
              setColourRgb(colorWheelR, colorWheelG, colorWheelB, colorWheelExtraLight, false, 1000);
              autoCtrl = false; //in theroy this is unnesecearry because colorwheel never issue full zero color
              colorWheel = true; //siwtch back colorWheel because setColourRGB disables it
              break;
          }
        //Serial.print(lastIR, HEX);
        //Serial.println(" - normal");
        normalmode = false;
      }
      repeat_counter = 0;
      lastIR=0;
    }
  }


  if (colorWheel == true && (millis() - lastColorWheelChange_millis) > (unsigned long)colorWheelChangeTime) { //COLOR WHEEL state code    
    switch(colorWheelWhichColorToDecrease) {
        case 0:
          colorWheelR--;
          colorWheelG++;
          if (colorWheelR == 0) {
            colorWheelNextColorToDecrease = 1;
          }       
        break;
        case 1:
          colorWheelG--;
          colorWheelB++;
          if (colorWheelG == 0) {
            colorWheelNextColorToDecrease = 2;
          } 
        break;
        case 2:
          colorWheelB--;
          colorWheelR++;
          if (colorWheelB == 0) {
            colorWheelNextColorToDecrease = 0;
          } 
        break;
    }
    colorWheelWhichColorToDecrease = colorWheelNextColorToDecrease;
    setColourRgbFastSimple(colorWheelR, colorWheelG, colorWheelB, colorWheelExtraLight);
    lastColorWheelChange_millis = millis();
  }
  
  
  //AUTOMATED CODE BASED ON PIR SENSOR and analogRead(0)
  if (autoCtrl == true) {
    if (digitalRead(pirPin) == HIGH && (analogRead(0) < 250 || autoOff == true) && (millis() - lastMovement_millis) > 1500) {
      if (autoOff != true) { //we dont need to set the MEM1 color again, it would take one sec so the normal IR detection would be sluggish if movement is detected
        Serial.println("AUTO turn on MEM1");
        Serial.println(analogRead(0));
        setColourRgb(EEPROM.read(3),EEPROM.read(4),EEPROM.read(5),EEPROM.read(6)); //automated mode uses the MEM1 settings
        autoCtrl = true; //set the auto control back to true because setColourRGB disables it
        autoOff = true; //set the automated off mode true
      }
      Serial.println("renew timer");
      lastMovement_millis = millis(); //last movement's timestamp
    }
    if (autoOff == true && (millis() - lastMovement_millis) > ((unsigned long)autoSwitchoffTimer * 1000)) {
      Serial.println("AUTO turn off lights");
      setColourRgb(0,0,0,0); //auto off mode is set by the setColourRgb(0,0,0,0)
      lastMovement_millis = millis(); //last movement's timestamp
    }
  }


} //end of main LOOP


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
  if (changeRef) {
    redRef = redSet;
    greenRef = greenSet;
    blueRef = blueSet;
    extraRef = extraSet;
  }
  //disable all automated/other states, except the AUTO light control will be ON if all colors are 0 (like OFF button, or just manual zero light)
  autoCtrl = redSet == 0 && greenSet == 0 && blueSet == 0 && extraSet == 0 ? true : false ;
  autoOff = false;
  colorWheel = false;
} 

void setColourRgbFastSimple(int redSet, int greenSet, int blueSet, int extraSet) {
  analogWrite(redPin, redSet);
  analogWrite(greenPin, greenSet);
  analogWrite(bluePin, blueSet);
  analogWrite(extraPin, extraSet);
  redCur = redSet;
  greenCur = greenSet;
  blueCur = blueSet;
  extraCur = extraSet;
}
