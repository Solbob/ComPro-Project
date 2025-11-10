#include <Servo.h>

#define WALL_DIST 10.0
#define FRONT_LIMIT 10.0

#define MOTOR_SPEED_FWD 100
#define MOTOR_SPEED_TURN 90

const int servoPins[] = {13, 12};
Servo servos[2];

const int motorPins[6] = {A2, 8, 9, 10, 11, A3}; // ENA, IN1, IN2, IN3, IN4, ENB

const int trigPins[] = {4, 2, A4};
const int echoPins[] = {5, 3, A5};

const int irPins[] = {6, 7};
const int irAnalogPins[] = {A0, A1};

struct SonarData {
  float front;
  float left;
  float right;
};

SonarData sonar;

float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 20000);
  float distance = duration * 0.0343 / 2.0;
  return (distance == 0) ? 400 : distance;
}

void readSonars(SonarData *s) {
  (*s).front = getDistance(trigPins[0], echoPins[0]);
  (*s).left  = getDistance(trigPins[1], echoPins[1]);
  (*s).right = getDistance(trigPins[2], echoPins[2]);

  Serial.print("Sonar -> Front: "); Serial.print((*s).front);
  Serial.print(" | Left: "); Serial.print((*s).left);
  Serial.print(" | Right: "); Serial.println((*s).right);
}

void moveForward(int speed) {
  analogWrite(motorPins[0], speed);
  analogWrite(motorPins[5], speed);
  digitalWrite(motorPins[1], HIGH);
  digitalWrite(motorPins[2], LOW);
  digitalWrite(motorPins[3], HIGH);
  digitalWrite(motorPins[4], LOW);
}

void moveBackward(int speed) {
  analogWrite(motorPins[0], speed);
  analogWrite(motorPins[5], speed);
  digitalWrite(motorPins[1], LOW);
  digitalWrite(motorPins[2], HIGH);
  digitalWrite(motorPins[3], LOW);
  digitalWrite(motorPins[4], HIGH);
}

void turnLeft(int speed) {
  analogWrite(motorPins[0], speed);
  analogWrite(motorPins[5], speed);
  digitalWrite(motorPins[1], LOW);
  digitalWrite(motorPins[2], HIGH);
  digitalWrite(motorPins[3], HIGH);
  digitalWrite(motorPins[4], LOW);
}

void turnRight(int speed) {
  analogWrite(motorPins[0], speed);
  analogWrite(motorPins[5], speed);
  digitalWrite(motorPins[1], HIGH);
  digitalWrite(motorPins[2], LOW);
  digitalWrite(motorPins[3], LOW);
  digitalWrite(motorPins[4], HIGH);
}

void stopMotors() {
  analogWrite(motorPins[0], 0);
  analogWrite(motorPins[5], 0);
  digitalWrite(motorPins[1], LOW);
  digitalWrite(motorPins[2], LOW);
  digitalWrite(motorPins[3], LOW);
  digitalWrite(motorPins[4], LOW);
}

bool grabbed = false;
unsigned long irDetectedTime = 0;
unsigned long ignoreIRUntil = 0;
unsigned long startTime = 0;

void grabSequence() {
  Serial.println("Grab sequence started!");
  moveForward(MOTOR_SPEED_FWD);
  delay(2000);
  stopMotors();

  servos[0].write(200);
  delay(800);
  servos[1].write(45);
  delay(1000);

  Serial.println("Object grabbed, ignoring IR for navigation");
  grabbed = true;
  ignoreIRUntil = millis() + 8000;
}

void wallFollow() {
  readSonars(&sonar);

  if (sonar.front < FRONT_LIMIT) {
    Serial.println("Wall ahead!");
    if (sonar.left > sonar.right) {
      Serial.println("Turning LEFT");
      turnLeft(MOTOR_SPEED_TURN);
      delay(300);
    } else {
      Serial.println("Turning RIGHT");
      turnRight(MOTOR_SPEED_TURN);
      delay(300);
    }
    stopMotors();
  } 
  else if (abs(sonar.left - sonar.right) <= 1.5) {
    Serial.println("Centered, move forward");
    moveForward(MOTOR_SPEED_FWD);
  } 
  else if (sonar.left > sonar.right + 2) {
    Serial.println("Too far from right wall -> Adjust RIGHT");
    turnRight(MOTOR_SPEED_TURN);
    delay(150);
    stopMotors();
  } 
  else if (sonar.right > sonar.left + 2) {
    Serial.println("Too far from left wall -> Adjust LEFT");
    turnLeft(MOTOR_SPEED_TURN);
    delay(150);
    stopMotors();
  } 
  else {
    Serial.println("Default move forward");
    moveForward(MOTOR_SPEED_FWD);
  }
}

void setup() {
  Serial.begin(9600);
  for(int i = 0; i < 6; i++) {
    pinMode(motorPins[i], OUTPUT);
  }
  for(int i = 0; i < 3; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
  for(int i = 0; i < 2; i++) {
    pinMode(irPins[i], INPUT);
  }

  for(int i = 0; i < 2; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(0);
  }

  startTime = millis();
  Serial.println("System initialized.");
}

void loop() {
  unsigned long now = millis();

  if (now - startTime < 2000) {
    moveForward(MOTOR_SPEED_FWD);
    return;
  }

  int irLeft = digitalRead(irPins[0]);
  int irRight = digitalRead(irPins[1]);
  int analogLeft = analogRead(irAnalogPins[0]);
  int analogRight = analogRead(irAnalogPins[1]);

  Serial.print("IR L:"); Serial.print(irLeft);
  Serial.print(" R:"); Serial.print(irRight);
  Serial.print(" | Analog L:"); Serial.print(analogLeft);
  Serial.print(" R:"); Serial.println(analogRight);

  readSonars(&sonar);

  if (!grabbed && now > ignoreIRUntil && irLeft == LOW && irRight == LOW && sonar.front < 15) {
    grabSequence();
  } 
  else if (grabbed) {
    wallFollow();
  } 
  else {
    moveForward(MOTOR_SPEED_FWD);
  }
}
