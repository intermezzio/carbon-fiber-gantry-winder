/* Stepper Motor Control */

#include <AccelStepper.h>
#define STEPS 200 // steps per revolution
#define CROSS 2000 // steps to cross the gantry

// change this to fit the number of steps per revolution
// for your motor
// initialize the stepper library on pins 8 through 11:
// create steppers
AccelStepper beltStepper(STEPS, 8, 9, 10, 11);
AccelStepper rotateStepper(STEPS, 4, 5, 6, 7);

// configuration for the job
struct Config {
  int beltSpeed;
  int rotateSpeed;
  int stepsAtEnd; // steps to rotate at the end of the tube
  int revolutions;
};

Config c = {
  60,
  60,
  100,
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
};

int startTime = millis();

void setup() {
  // TODO: zero the motors
  
  // set the speed at 60 rpm:
  // initialize the serial port:
  Serial.begin(9600);
}

bool checkEmergencyPause() {
  /** If an e-stop-esque button is pressed
   *  return true, else return false
   *  TODO: implement
   */
  return false;
}

bool checkRestart() {
  /** If the code is paused return true
   *  if the restart button is pressed
   *  TODO: implement
   */
  return false;
}

// this is necessary for calling the function later
void driveMotors();

void altDelay() {
  /** Delay but check for emergency stop
   *  during the downtime
   *  Wait until given stepper stops moving
   */
  while(gs.mainStepper.isRunning()) {
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
      driveMotors();
    }

    if(&gs.mainStepper != &rotateStepper && !rotateStepper.isRunning()) {
      rotateStepper.move(99999999); // this should always move
    }
  }
  rotateStepper.stop();
  gs.steps = 0;
  return;
}

void driveMotors() {
  /** Use the global GantryState object
   *  to set motor speeds
   *  TODO: set motor speeds
   */
  // set motor speeds
  gs.mainStepper.move(gs.steps);
  // delay
  altDelay();
  // delay(5000);
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
      break;
    case 'r': // reverse
      gs.steps = -CROSS;
      gs.mainStepper = beltStepper;
      break;
    case 'e': // end
      gs.steps = c.stepsAtEnd;
      gs.mainStepper = rotateStepper;
      break;
    default:
      break;
  }
  
  // drive motors at altered gs
  driveMotors();
  
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
    gs.state = 'e';
    moveGantry();
  }

  Serial.println("job finished.");
  while(true){};
}
