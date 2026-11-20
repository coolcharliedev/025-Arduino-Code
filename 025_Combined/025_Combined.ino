#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AFMotor.h>
#include <Servo.h>
#include "GravityTDS.h" // Custom Library provided by manufacturer
// STATE DEFINITION

int standBy = 0;
int patientToTest = 1;
int dispensingAmylase = 2;
int filtering = 3;
int detection = 4;
int refilter = 5;
int finish = 6;

// GLOBAL VARIABLE DECLARATION

int cycleCounter = 0;
int currentState = 0;
float tdsValue;
float liquidLevel;
long startOfPPumpPumpingVersion1First;


// CONSTANT VARIABLE DECLARATION

// DC MOTOR
AF_DCMotor dcmotor(1);

//CONFIG SPUMP
AF_DCMotor spump(2);

//CONFIG DPUMP
AF_DCMotor dpump(3);

//CONFIG ppump
AF_DCMotor ppump(4);

// Define pins
  int LLpin = 30;

//The OLEDs you have in your kit are 0.96" I2C 128x64 Monochrome OLEDs
// 0.96" is the screen size, I2C is the protocol it uses to talk to the Arduino
#define SCREEN_WIDTH 128 // OLED display width, in pixels. 
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define OLED_I2C_ADDRESS 0x3C // !!IMPORTANT!!: If OLED does not show any text, switch this value to 0x3D

#define startPin 24
#define endPin 25


Servo Servo1;



#define tdsPin A2   // Define the pin the TDS sensor is connected to. Important: You MUST use an Analog Pin (A0-A9)
GravityTDS tds;     // Create object of GravityTDS class, same as you do for the motor/pumps

void displayOled (String header, String line1, float value, String next){

  /*
  * You can display things from loop() for from setup(). 
  * But remember: You need to call display.clear(); at the start of loop or to change the value
  * Or else, it will keep displaying the same thing multiple times.
  */

  //clearDisplay(); is called to clear the screen of any text. You can use this everytime you need to refresh the screen
  display.clearDisplay(); 

  display.setTextSize(2);             // Control the size of text. You can use a value of 1,2,3 etc. 
  display.setTextColor(SSD1306_WHITE); // Control the colour of the text. The display is Monochrome, so you can only use white.

  /* setCursor(x,y). This determines the position x,y coordinate where your text will be placed on the screen
  * The Display is a 128x64 display. Top left corner is (0,0), bottom right corner is (128,64) */
  display.setCursor(0,0);       

 /* !!IMPORTANT!!
 * If you notice carefully, the following function has string wrapped in the macro F()
 * This is used for static text. It saves the text directly into flash memory on Arduino
 * which is used for storing the program (Program Storage Space)
 * If you do not use F() for static text, it will be copied into SRAM which takes up too much space unnecessarily
 * 
 * NOTE: You must NOT use F() for variable text i.e value of pH, TDS, Turbidity etc. 
 */
  display.println((header));  // This is example of static text. Its a simple string and does not change


  display.setTextSize(1); // Display the next line in bigger size
  display.println((line1));  // This is also static Text but specific to current state
  if(value >= 0){
    display.println(value);  // This is a variable text. It will keep changing
  }

  if(next){
    display.println((next));
  }
  

  display.display(); // Actually draw everything on the screen.
}

void setup() {

  spump.setSpeed(0); // Just check how the pump works. You can increase or decrease the speed same as other motors/pumps.
  spump.run(RELEASE);

  pinMode(startPin, INPUT_PULLUP); 
  pinMode(endPin, INPUT_PULLUP); 

  tds.setPin(tdsPin);
  tds.setAref(5.0);       // The sensor needs to have the reference voltage for TDS calculation. For Arduino Mega, it is 5V

  // This defines the range of the Analog-to-Dgitial Converter (ADC) which reads the value using analogRead()
  // For Arduino Mega, its a 10-bit ADC, so 1024 values
  tds.setAdcRange(1024);  
  tds.begin();
  

  Serial.begin(9600);

  //CONFIG OLED DISPLAY

  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever if the OLED did not initialize
  }

  //CONFIG SERV

  Servo1.attach(9); // Pin 9 corresponds to SERVO_2 and Pin 10 corresponds to SER1 connector on the shield


}

void loop() {
  if(digitalRead(endPin) == LOW){
    //EMERGENCY STOP
    dcmotor.run(RELEASE);
    spump.run(RELEASE);
    dpump.run(RELEASE);
    ppump.run(RELEASE);
    Servo1.writeMicroseconds(1482);
    currentState = 0;
  }
  /*if(digitalRead(startPin) == LOW){
    currentState++;
  }*/
  // STATE 0
  if (currentState == 0){
    displayOled("Standby", "Press start", -1, "");

    // Check if START button is pressed
    if(digitalRead(startPin) == LOW)            // Check if the button is pressed Pin used is A5. You can use any Pin. 
    {
      
        currentState = patientToTest;
      
    }
    
  }
  // STATE 1
  else if (currentState == 1){
    
    if(digitalRead(LLpin)==HIGH){
      displayOled("Pumping", "Pumping to test", -1, "");

      spump.setSpeed(220); // Just check how the pump works. You can increase or decrease the speed same as other motors/pumps.
      spump.run(FORWARD);
    }else{
      spump.setSpeed(0); // Just check how the pump works. You can increase or decrease the speed same as other motors/pumps.
      spump.run(FORWARD);

      currentState = dispensingAmylase;
    }
  }
  // STATE 2
  else if (currentState == 2){
    displayOled("Dispensing", "Dispensing & Mixing", -1, "");
    //BEGIN STIRRING
    dcmotor.setSpeed(230);          // Set the motor speed here. The motor speed is a 8 bit number 0-255. Where 0 = 0% speed and 255 = 100% speed.
    dcmotor.run(FORWARD);


    //ROTATE SERVO
    Servo1.write(60);
    delay(540);
    Servo1.writeMicroseconds(1482);
    delay(2000);

    Servo1.write(120);
    delay(540);
    Servo1.writeMicroseconds(1482);
    //
    {
      currentState = filtering;
      startOfPPumpPumpingVersion1First = millis();
      dcmotor.setSpeed(0);          // Set the motor speed here. The motor speed is a 8 bit number 0-255. Where 0 = 0% speed and 255 = 100% speed.
      dcmotor.run(RELEASE);
    }
  }
  // STATE 3
  else if (currentState == 3){

    tds.update();
    float tdsVal = tds.getTdsValue(); 
    
    ppump.setSpeed(255);
    ppump.run(FORWARD);
    long ctime = millis();
    //continue running until necessary amoung of time elapsed

    if(ctime - startOfPPumpPumpingVersion1First < (1000*60*2)){
      currentState = detection;
    }else{
      displayOled("Filtering", " Elapsed: ", ((ctime - startOfPPumpPumpingVersion1First)/(1000*60)), "");
    }
  }
  // STATE 4
  else if (currentState == 4){
    ppump.run(RELEASE);
    tds.update();
    float tdsVal = tds.getTdsValue(); 
    displayOled("Detecting TDS", "TDS VALUE: ", tdsVal, "ppm");

    delay(1000);

    currentState = 5;
  }
  // STATE 5
  else if (currentState == 5){
    tds.update();
    float tdsVal = tds.getTdsValue(); 
    
    ppump.setSpeed(240);
    ppump.run(BACKWARD);
    long ctime = millis();
    //continue running until necessary amoung of time elapsed

    if(ctime - startOfPPumpPumpingVersion1First < (1000*60*2)){
      ppump.run(RELEASE);
      if(cycleCounter >= 2){
        currentState = 6;
      }else{
        currentState = detection;
        cycleCounter++;
      }
      
    }else{
      displayOled("REVERSE Filtering", " Cycle", cycleCounter, "");
    }

    
  }
  // STATE 6
  else if (currentState == 6){
    dpump.setSpeed(230);
    dpump.run(FORWARD);

    displayOled("Finishing", "Returning blood to patient", -1, "");

    delay(20000);
    dpump.run(RELEASE);  
    }else{
      delay(1000);
    }
}
