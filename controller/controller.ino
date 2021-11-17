/* Stepper Motor Control */

#include <AccelStepper.h>
#include <MultiStepper.h>
#define STEPS 200 // steps per revolution
#define WIDTH 100 // width of final product in mm 
#define BELT_CIRC 84 // circumference of the roller on the belt
#define CROSS WIDTH * STEPS / 84 // steps to cross the gantry

// 26.65 mm diameter
// 26.65 * pi is about 84

// change this to fit the number of steps per revolution
// for your motor
// initialize the stepper library on pins 8 through 11:
// create steppers
//Stepper temp(STEPS, 4, 5, 6, 7);
AccelStepper beltStepper(AccelStepper::FULL4WIRE, 4, 5, 6, 7);
AccelStepper rotateStepper(AccelStepper::FULL4WIRE, 8, 9, 10, 11);
MultiStepper steppers;

// configuration for the job
struct Config {
  int pitch; // may add offset
  int beltSpeed;
  int rotateSpeed;
  int stepsAtEnd; // steps to rotate at the end of the tube
  int stepsForWidth;
  int revolutions;
};

Config c = {
  100,
  100,
  100,
  100,
  10,
  5
};

// store the state of the gantry in case of pause 
struct GantryState {
  char state;
  int steps;
  AccelStepper mainStepper; // stepper controlling timing or movement
};

GantryState gs = {
  'f',
  CROSS,
  beltStepper
};

long positions[2] = {0, 0};

int startTime = millis();

void setup() {
  beltStepper.setMaxSpeed(60);
  rotateStepper.setMaxSpeed(60);
  beltStepper.setSpeed(60);
  rotateStepper.setSpeed(60);
  
  steppers.addStepper(beltStepper);
  steppers.addStepper(rotateStepper);
  
  // set the speed at 60 rpm:
  // initialize the serial port:
  Serial.begin(9600);
}

void getConfig() {
  /** Ask Serial for configuration
   *  of the job
   */

  Serial.println("Type in the pitch for rolling (mm per revolution)");
  while(!Serial.available()) {}
  c.pitch = Serial.parseInt();
  Serial.println("Type in the number of revolutions");
  c.revolutions = Serial.parseInt();
}

void calculateSpeeds() {
  /** Given current preferences
   *  calculate speeds for the job
   */
  float speedRatio = (float)c.pitch / BELT_CIRC;
}

bool checkEmergencyPause() {
  /** If an e-stop-esque button is pressed
   *  return true, else return false
   *  TODO: implement
   */
  if(Serial.available() && Serial.read() == '0') {
    return true;
  }
  return false;
}

bool checkRestart() {
  /** If the code is paused return true
   *  if the restart button is pressed
   *  TODO: implement
   */
  return false;
}

void altDelay() {
  /** Delay but check for emergency stop
   *  during the downtime
   *  Wait until given stepper stops moving
   */
  Serial.println("Started delay");

  while(gs.mainStepper.distanceToGo() != 0) {
    steppers.run();
    Serial.println(gs.mainStepper.distanceToGo());
    
    // if stop button pressed, stop!
    if(checkEmergencyPause()) {
      // store state
      gs.steps = gs.mainStepper.distanceToGo();

      // stop motors
      beltStepper.stop();
      rotateStepper.stop();
      
      // await a restart signal
      while(!checkRestart()) {}

      // restart motors
      steppers.moveTo(positions); // TODO: fix?
      return altDelay();
      
    }

    
  }
  rotateStepper.stop();
  gs.steps = 0;
  return;
}

void moveGantry() { 
  /** Determine speeds and move gantry in 
   *  the direction of choosing
   */
  // change steps left given direction
  switch(gs.state) {
    case 'f': // forward
      gs.steps = CROSS;
      gs.mainStepper = beltStepper;
      positions[0] = CROSS;
      positions[1] = 99999; // far enough
      break;
    case 'r': // reverse
      gs.steps = -CROSS;
      gs.mainStepper = beltStepper;
      positions[0] = 0;
      positions[1] = 99999; // far enough
      break;
    case 'e': // end
      gs.steps = c.stepsAtEnd;
      gs.mainStepper = rotateStepper;
      rotateStepper.setCurrentPosition(0);
      positions[1] = c.stepsAtEnd;
      break;
    case 'b': // beginning (near end)
      gs.steps = c.stepsAtEnd;
      gs.mainStepper = rotateStepper;
      rotateStepper.setCurrentPosition(0);
      positions[1] = c.stepsAtEnd + c.stepsForWidth;
    default:
      break;
  }
  Serial.print("Gantry steps ");
  Serial.println(gs.steps);
  // drive motors at altered gs
  steppers.moveTo(positions);
  altDelay();
  
  return;
}

void printElapsedTime() {
  Serial.print("Elapsed time: ");
  int msPast = millis() - startTime;
  int msRemainder = msPast % 1000;
  int sPast = msPast / 1000;
  int sRemainder = sPast % 60;
  int mPast = sPast / 60;
  Serial.print(mPast);
  Serial.print("m ");
  Serial.print(sRemainder);
  Serial.print(".");
  Serial.print(msRemainder);
  Serial.println("s");
  return;
}

void loop() {
  // Get job details and store speeds
  // getConfig();

  beltStepper.setSpeed(c.beltSpeed);
  rotateStepper.setSpeed(c.rotateSpeed);
  
  for(int i = 0; i < c.revolutions; i++) {
    Serial.print("Starting iteration ");
    Serial.print(i+1);
    Serial.print(" of ");
    Serial.println(c.revolutions);
    printElapsedTime();
    
    // forward
    Serial.println("Forward");
    gs.state = 'f';
    moveGantry();
    
    // end
    Serial.println("Pausing at the far end");
    gs.state = 'e';
    moveGantry();
    
    // reverse
    Serial.println("Reverse");
    gs.state = 'r';
    moveGantry();
    
    // end
    Serial.println("Pausing at the near end");
    gs.state = 'b';
    moveGantry();
  }

  Serial.println("Job finished.");
  while(true){};
}
