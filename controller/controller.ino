/* Stepper Motor Control */

// #include <Stepper.h>
const int stepsPerRevolution = 90;
const int timeToCross = 10000; // ms to cross the gantry
const int timeAtEnd = 2000; // ms to cross the gantry

// change this to fit the number of steps per revolution
// for your motor
// initialize the stepper library on pins 8 through 11:
// create steppers
// Stepper beltStepper(stepsPerRevolution, 8, 9, 10, 11);
// Stepper rotateStepper(stepsPerRevolution, 1, 2, 3, 4);

// configuration for the job
struct Config {
  int beltSpeed;
  int rotateSpeed;
  int revolutions;
};

Config c = {
  3,
  3,
  100
};

// store the state of the gantry in case of pause 
struct GantryState {
  int beltSpeed;
  int rotateSpeed;
  int duration;
};

GantryState gs = {
  3,
  3,
  0
};

void setup() {
  // TODO: zero the motors
  
  // set the speed at 60 rpm:
  // myStepper.setSpeed(5);
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
   */
  int endTime = gs.duration + millis();
  
  // if stop button pressed, stop!
  while(millis() < endTime) {
    if(checkEmergencyPause()) {
      // store state
      gs.duration = endTime - millis();

      // await a restart signal
      while(!checkRestart()) {}

      // redefine end time
      int endTime = gs.duration + millis();
      
      // restart motors
      driveMotors();
    }
  }
  gs.duration = 0;
  return;
}

void driveMotors() {
  /** Use the global GantryState object
   *  to set motor speeds
   *  TODO: set motor speeds
   */
  // set motor speeds
  
  // delay
  altDelay();
  return;
}

void moveGantry(char direction) { 
  /** Determine speeds and move gantry in 
   *  the direction of choosing
   */
  // change gs (GantryState) given direction
  switch(direction) {
    case 'f': // forward
      gs.duration = timeToCross;
      break;
    case 'r': // reverse
      gs.duration = timeToCross;
      break;
    case 'e': // end
      gs.duration = timeAtEnd;
      break;
    default:
      break;
  }
  // drive motors at altered gs
  driveMotors();
}


void loop() {
  // Get job details and store speeds
  // getConfig();

  for(int i = 0; i < c.revolutions; i++) {
      // forward
      moveGantry('f');
      
      // end
      moveGantry('e');
    
      // reverse
      moveGantry('r');
      
      // end
      moveGantry('e');
  }

  /* garbage code
  // step one revolution in one direction:
  Serial.println("clockwise");
  myStepper.step(stepsPerRevolution);
  delay(500);
  // step one revolution in the other direction:
  Serial.println("counterclockwise");
  myStepper.step(-stepsPerRevolution);
  delay(500);
   */
}