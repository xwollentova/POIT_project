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

    // ignoruj CR/LF
    if (cmd == '\n' || cmd == '\r') return;

    // premeni malé písmená na veľké
    cmd = toupper(cmd);

    // DEBUG: pozri presne, čo prišlo
    Serial.print("Got cmd: ");
    Serial.println(cmd);

    switch (cmd) {
      case 'F':  // vpred
        motor_1.run(FORWARD);
        motor_2.run(FORWARD);
        motor_3.run(FORWARD);
        motor_4.run(FORWARD);
        break;

      case 'B':  // vzad
        motor_1.run(BACKWARD);
        motor_2.run(BACKWARD);
        motor_3.run(BACKWARD);
        motor_4.run(BACKWARD);
        break;

      case 'L':  // vľavo
        motor_1.run(FORWARD);
        motor_4.run(FORWARD);
        motor_2.run(RELEASE);
        motor_3.run(RELEASE);
        break;

      case 'R':  // vpravo
        motor_2.run(FORWARD);
        motor_3.run(FORWARD);
        motor_1.run(RELEASE);
        motor_4.run(RELEASE);
        break;

      case 'S':  // STOP motory
        motor_1.run(RELEASE);
        motor_2.run(RELEASE);
        motor_3.run(RELEASE);
        motor_4.run(RELEASE);
        break;

      case 'M':  // meranie START – iba debug, motory sa nespustia ani nezastavia
        Serial.println("Received MEASURE_START (ignored by Arduino)");
        break;

      case 'X':  // meranie STOP – iba debug
        Serial.println("Received MEASURE_STOP (ignored by Arduino)");
        break;

      default:   // čokoľvek iné (napr. nezrozumiteľné znaky)
        Serial.print("Unknown cmd, ignoring: ");
        Serial.println(cmd);
        break;
    }
  }
}

