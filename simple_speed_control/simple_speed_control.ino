// ConstantSpeed.pde
// -*- mode: C++ -*-
//
// Shows how to run AccelStepper in the simplest,
// fixed speed mode with no accelerations
/// \author  Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2009 Mike McCauley
// $Id: ConstantSpeed.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

#include <AccelStepper.h>
#include <MultiStepper.h>
#define STEPS 200 // steps per revolution
#define BELT_CIRC 84 // circumference of the roller on the belt
#define HALF_STEPS STEPS / 2 // 180 degrees

// --- ADD CONFIGURATION HERE

#define LENGTH 200. // length of tube in mm
#define RADIUS 19.1 // radius of winding tube in mm
//#define PITCH 50.1 // pitch of winding in mm, define this or ANGLE
#define ANGLE 25. // angle in degrees (0-90) of increase (0 degrees is vertical)
#define THICKNESS 0.4 // thickness of filament in mm
#define ITERATIONS 15 // number of iterations
#define MAX_SPEED 175 // maximum stepper speed

// --- CONFIGURATION ENDS HERE

int CROSS = (int)(LENGTH * STEPS / 84); // steps to cross the gantry
#ifndef PITCH
float PITCH = (PI * tan(ANGLE * PI / 180) * 2 * RADIUS);
#endif

AccelStepper beltStepper(AccelStepper::FULL4WIRE,4,5,6,7); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
AccelStepper rotateStepper(AccelStepper::FULL4WIRE,8,9,10,11); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

MultiStepper steppers;

long positions[2] = {CROSS, 0};

int iteration = 0;

float SPEED_RATIO;
int BELT_SPEED;
int TUBE_SPEED;
int CROSS_TURN_STEPS; // how many steps to rotate when the belt crosses
int FULL_CIRCLE_STEPS; // how many steps to rotate after an iteration to line up the rotating motor
int AFTER_ITERATION_STEPS; // how many steps to turn to move the carbon fiber over for the next iteration

void setup() {  
  calculateConstants();
  
  beltStepper.setMaxSpeed(BELT_SPEED);
  beltStepper.setSpeed(BELT_SPEED);	
  rotateStepper.setMaxSpeed(TUBE_SPEED);
  rotateStepper.setSpeed(TUBE_SPEED); 

  // Then give them to MultiStepper to manage
  steppers.addStepper(beltStepper);
  steppers.addStepper(rotateStepper);

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

  delay(1000);
  
  Serial.println("First Roll");
  positions[0] = 0;
  positions[1] = STEPS;
  steppers.moveTo(positions);
  while(rotateStepper.distanceToGo() != 0) {
    Serial.println(rotateStepper.distanceToGo());
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

void calculateConstants() {
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
  CROSS_TURN_STEPS = (int) (CROSS / SPEED_RATIO);

  // steps to take after one iteration
  // to line up the sample again
  float percLeft = 1 - (PITCH - fmod(2*LENGTH, PITCH)) / PITCH;
  FULL_CIRCLE_STEPS = (int) (STEPS * percLeft);


  // steps to take after lining up the sample
  // to offset the next iteration
  AFTER_ITERATION_STEPS = (int) (STEPS * THICKNESS / PITCH);

}

void loop() {  
  if(++iteration == ITERATIONS) {
    Serial.println("Job completed");
    beltStepper.stop();
    rotateStepper.stop();
    while(true) {}
  }
  
  Serial.print("Iteration ");
  Serial.println(iteration);

  Serial.println("Forward!!!");
  positions[0] = CROSS;
  positions[1] = CROSS_TURN_STEPS;

  steppers.moveTo(positions);
  while(beltStepper.distanceToGo() != 0) {
    Serial.println(beltStepper.distanceToGo());      
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
  
  Serial.println("Far end!!!");
  positions[1] = HALF_STEPS; // + FULL_CIRCLE_STEPS/2;
  steppers.moveTo(positions);
  while(rotateStepper.distanceToGo() != 0) {
    Serial.println(rotateStepper.distanceToGo());
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
  
  Serial.println("Reverse!!!");
  positions[0] = 0;
  positions[1] = CROSS_TURN_STEPS;
  
  steppers.moveTo(positions);
  while(beltStepper.distanceToGo() != 0) {
    Serial.println(beltStepper.distanceToGo());
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);

  Serial.println("Near end!!!");
  positions[1] = HALF_STEPS + AFTER_ITERATION_STEPS; // + FULL_CIRCLE_STEPS/2 + 
  steppers.moveTo(positions);
  while(rotateStepper.distanceToGo() != 0) {
    Serial.println(rotateStepper.distanceToGo());
    steppers.run();
  }
  rotateStepper.setCurrentPosition(0);
}
