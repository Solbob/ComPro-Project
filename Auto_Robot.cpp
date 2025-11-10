#include <Servo.h>




#define SERVO_GRAB_PIN 13
#define SERVO_LIFT_PIN 12


#define ENA A2
#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11
#define ENB A3

 
#define TRIG_FRONT 4
#define ECHO_FRONT 5
#define TRIG_LEFT 2
#define ECHO_LEFT 3
#define TRIG_RIGHT A4
#define ECHO_RIGHT A5


#define IR_LEFT 6
#define IR_RIGHT 7
#define IR_LEFT_ANALOG A0
#define IR_RIGHT_ANALOG A1

  
#define WALL_DIST 10.0
#define FRONT_LIMIT 10.0

#define MOTOR_SPEED_FWD 100
#define MOTOR_SPEED_TURN 90
   
Servo servoGrab;
Servo servoLift;

bool grabbed = false;
unsigned long irDetectedTime = 0;
unsigned long ignoreIRUntil = 0;
unsigned long startTime = 0;

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
  (*s).front = getDistance(TRIG_FRONT, ECHO_FRONT);
  (*s).left  = getDistance(TRIG_LEFT, ECHO_LEFT);
  (*s).right = getDistance(TRIG_RIGHT, ECHO_RIGHT);

  Serial.print("Sonar -> Front: "); Serial.print((*s).front);
  Serial.print(" | Left: "); Serial.print((*s).left);
  Serial.print(" | Right: "); Serial.println((*s).right);
}

// MOTOR CONTROL
void moveForward(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// SERVO CONTROL
void grabSequence() {
  Serial.println("Grab sequence started!");
  moveForward(MOTOR_SPEED_FWD);
  delay(2000);
  stopMotors();

  servoGrab.write(200); //close grabber
  delay(800);
  servoLift.write(45);  //lift up
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
 
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

   
  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);

  servoGrab.attach(SERVO_GRAB_PIN);
  servoLift.attach(SERVO_LIFT_PIN);

  servoGrab.write(0);
  servoLift.write(0);

  startTime = millis();
  Serial.println("System initialized.");
}

   
void loop() {
  unsigned long now = millis();

  // Ignore IR for first 2 seconds
  if (now - startTime < 2000) {
    moveForward(MOTOR_SPEED_FWD);
    return;
  }

  // Read IR
  int irLeft = digitalRead(IR_LEFT);
  int irRight = digitalRead(IR_RIGHT);
  int analogLeft = analogRead(IR_LEFT_ANALOG);
  int analogRight = analogRead(IR_RIGHT_ANALOG);

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
