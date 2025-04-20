#include <AFMotor.h>
#include<Arduino.h>

AF_DCMotor motor_1(1);
AF_DCMotor motor_2(2);
AF_DCMotor motor_3(3);
AF_DCMotor motor_4(4);

void setup() {
  Serial.begin(9600);
  motor_1.setSpeed(255);
  motor_2.setSpeed(255);
  motor_3.setSpeed(255);
  motor_4.setSpeed(255);
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'F') {
      motor_1.run(FORWARD);
      motor_2.run(FORWARD);
      motor_3.run(FORWARD);
      motor_4.run(FORWARD);
    } else if (cmd == 'B') {
      motor_1.run(BACKWARD);
      motor_2.run(BACKWARD);
      motor_3.run(BACKWARD);
      motor_4.run(BACKWARD);
    } else if (cmd == 'L') {
      motor_1.run(FORWARD);
      motor_4.run(FORWARD);
      motor_2.run(RELEASE);
      motor_3.run(RELEASE);
    } else if (cmd == 'R') {
      motor_2.run(FORWARD);
      motor_3.run(FORWARD);
      motor_1.run(RELEASE);
      motor_4.run(RELEASE);
    } else if (cmd == 'S') {
      motor_1.run(RELEASE);
      motor_2.run(RELEASE);
      motor_3.run(RELEASE);
      motor_4.run(RELEASE);
    }
  }
}
