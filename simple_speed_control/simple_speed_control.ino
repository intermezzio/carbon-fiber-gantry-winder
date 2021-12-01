#include <AccelStepper.h>
#include <MultiStepper.h>
#include <LiquidCrystal_I2C.h>

#define STEPS 200 // steps per revolution
#define BELT_CIRC 84 // circumference of the roller on the belt
#define HALF_STEPS (STEPS / 2) // 180 degrees

// --- ADD CONFIGURATION HERE

#define LENGTH 100. // length of tube in mm
#define RADIUS 16.75 // radius of winding tube in mm
//#define PITCH 50.1 // pitch of winding in mm, define this or ANGLE
#define HELICAL_ANGLE 25. // angle in degrees (0-90) of increase (0 degrees is vertical)
#define THICKNESS 2.5 // thickness of filament in mm
//#define ITERATIONS 5 // number of iterations
#define MAX_SPEED 100 // maximum stepper speed

// --- CONFIGURATION ENDS HERE

#define CIRCUMFERENCE (2*PI*RADIUS)
float HOOP_ANGLE = (atan(THICKNESS / CIRCUMFERENCE) * 180 / PI); // manually divide by 2
int CROSS = (int)(LENGTH * STEPS / 84); // steps to cross the gantry
float ANGLE = HOOP_ANGLE;
#ifndef PITCH
float PITCH = (PI * tan(ANGLE * PI / 180) * 2 * RADIUS);
#endif

int ITERATIONS = 10;

char buffer[16];
int PAUSE = 13;
int RESUME = 13;

AccelStepper beltStepper(AccelStepper::FULL4WIRE,4,5,6,7); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
AccelStepper rotateStepper(AccelStepper::FULL4WIRE,8,9,10,11); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

MultiStepper steppers;

LiquidCrystal_I2C lcd(0x27,16,2);

long positions[2] = {CROSS, 0};

int iteration = 0;

float SPEED_RATIO;
int BELT_SPEED;
int TUBE_SPEED;
int CROSS_TURN_STEPS; // how many steps to rotate when the belt crosses
int FULL_CIRCLE_STEPS; // how many steps to rotate after an iteration to line up the rotating motor
int AFTER_ITERATION_STEPS; // how many steps to turn to move the carbon fiber over for the next iteration

float JANK_CROSS_TUBE_DOPING_MULTIPLIER = 8./7;

const bool BELT = true;
const bool ROTATE = false;

void setup() {  
  lcd.init();
  calculateConstants();
  
  // Then give them to MultiStepper to manage
  steppers.addStepper(beltStepper);
  steppers.addStepper(rotateStepper);

  pinMode(PAUSE, INPUT);
  pinMode(RESUME, INPUT);
  
  Serial.begin(9600);

  Serial.print(ITERATIONS);
  Serial.println(" iterations");

  Serial.print("Full circle steps: ");
  Serial.println(FULL_CIRCLE_STEPS);
  
  Serial.print("AFTER_ITERATION_STEPS ");
  Serial.println(AFTER_ITERATION_STEPS);

  Serial.print("CROSS ");
  Serial.println(CROSS);

  Serial.print("SPEED_RATIO ");
  Serial.println(SPEED_RATIO);

  Serial.print("CROSS_TURN_STEPS ");
  Serial.println(CROSS_TURN_STEPS);

  Serial.print("PITCH ");
  Serial.println(PITCH);

  Serial.print("BELT_SPEED ");
  Serial.println(BELT_SPEED);

  Serial.print("TUBE_SPEED ");
  Serial.println(TUBE_SPEED);

  Serial.print("HOOP_ANGLE ");
  Serial.println(HOOP_ANGLE);

  Serial.print("PITCH ");
  Serial.println(PITCH);
  
  delay(1000);
  
  Serial.println("First Roll");
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("  1/");
  lcd.setCursor(4,0);
  printNumSpace(ITERATIONS,3);
  lcd.print("it ");
  lcd.print("end");
  
  lcd.setCursor(5,1);
  lcd.print("steps left");
  
  
  positions[0] = 0;
  positions[1] = STEPS;
  steppers.moveTo(positions);
  while(rotateStepper.distanceToGo() != 0) {
    printLcdSteps(rotateStepper.distanceToGo());
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
}

float fmod(float x, float y) {
  while(true) {
    if(x < 0) {
      return x+y;
    }
    x -= y;
  }
  return 0;
}

void printLcdSteps(long steps) {
  lcd.setCursor(0,1);
  printNumSpace(steps, 4);
}

void printNumSpace(long num, int size) {
  int len = 1;
  long temp = num;
  while ( temp /= 10 ) {
    len++;
  }
  for(int i = 0; i < size-len; i++) {
    lcd.print(" ");
  }
  lcd.print(num);
}

void calculateConstants() {
  PITCH = (PI * tan(ANGLE * PI / 180) * 2 * RADIUS);
  
  // ratio of speeds of both motors
  SPEED_RATIO = PITCH / BELT_CIRC;
  if(SPEED_RATIO < 1) {
    TUBE_SPEED = MAX_SPEED; // tube faster
    BELT_SPEED = (int) (TUBE_SPEED * SPEED_RATIO);
  } else {
    BELT_SPEED = MAX_SPEED;
    TUBE_SPEED = (int)(BELT_SPEED / SPEED_RATIO);
  }
  
  // steps rotation motor takes when belt motor crosses
  CROSS_TURN_STEPS = (int) (CROSS / SPEED_RATIO * JANK_CROSS_TUBE_DOPING_MULTIPLIER);

  // steps to take after one iteration
  // to line up the sample again
  float percLeft = 1 - (PITCH - fmod(2*LENGTH, PITCH)) / PITCH;
  FULL_CIRCLE_STEPS = (int) (STEPS * percLeft);


  // steps to take after lining up the sample
  // to offset the next iteration
  AFTER_ITERATION_STEPS = (int) (THICKNESS / cos(PI * HELICAL_ANGLE / 180) * STEPS / (2*PI*RADIUS));

  ITERATIONS = STEPS / AFTER_ITERATION_STEPS + 1;
  
  beltStepper.setMaxSpeed(BELT_SPEED);
  beltStepper.setSpeed(BELT_SPEED);  
  rotateStepper.setMaxSpeed(TUBE_SPEED);
  rotateStepper.setSpeed(TUBE_SPEED); 
}

bool pause() {
  return digitalRead(PAUSE);
}
bool resume() {
  return digitalRead(RESUME);
}

long statusChecks(bool printBelt) {
  /** Check if a pause was requested
   * Print to Serial (and LCD)
   */
  long beltStepsToGo = beltStepper.distanceToGo();
  long rotateStepsToGo = rotateStepper.distanceToGo();
  
  // check for pause
  if(pause()) {
    Serial.println("Pause!");
    beltStepper.stop();
    rotateStepper.stop();
    
    delay(1000);
    // check for resume
    while(!resume()) {}
    
    Serial.println("Resume!");
    delay(1000);
    
    long tempPositions[2] = {positions[0], positions[1]};
    steppers.moveTo(tempPositions);
    
  }
  long stepsToPrint = abs(printBelt ? beltStepsToGo : rotateStepsToGo);
  Serial.println(stepsToPrint);
  printLcdSteps(stepsToPrint);
  return stepsToPrint;
}

void loop() {  
  iteration++;
  if(iteration == 1) {
    ANGLE = HOOP_ANGLE;
    calculateConstants();
  } else if(iteration == 2) {
    ANGLE = HELICAL_ANGLE;
    calculateConstants();
  } else if(iteration == ITERATIONS) {
    Serial.println("Job completed");
    beltStepper.stop();
    rotateStepper.stop();
    while(true) {}
  }

  Serial.print("TUBE_SPEED ");
  Serial.println(TUBE_SPEED);

  Serial.print("BELT_SPEED ");
  Serial.println(BELT_SPEED);
  
  Serial.print("Iteration ");
  Serial.print(iteration);
  Serial.print(" of ");
  Serial.println(ITERATIONS);

  lcd.setCursor(0,0);
  printNumSpace(iteration, 3);
  
  Serial.println("Forward!!!");
  lcd.setCursor(10,0);
  lcd.print("fwd");
  positions[0] = CROSS;
  positions[1] = CROSS_TURN_STEPS;

  steppers.moveTo(positions);
  while(statusChecks(BELT) != 0) {
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
  
  Serial.println("Far end!!!");
  lcd.setCursor(10,0);
  lcd.print("end");
  positions[1] = STEPS + HALF_STEPS + FULL_CIRCLE_STEPS/2;
  steppers.moveTo(positions);
  while(statusChecks(ROTATE) != 0) {
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
  
  Serial.println("Reverse!!!");
  lcd.setCursor(10,0);
  lcd.print("rev");
  positions[0] = 0;
  positions[1] = CROSS_TURN_STEPS;
  
  steppers.moveTo(positions);
  while(statusChecks(BELT) != 0) {
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);

  Serial.println("Near end!!!");
  lcd.setCursor(10,0);
  lcd.print("end");
  positions[1] = STEPS + HALF_STEPS + FULL_CIRCLE_STEPS/2 + AFTER_ITERATION_STEPS; 
  steppers.moveTo(positions);
  while(statusChecks(ROTATE) != 0) {
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
}
