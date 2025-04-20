#include <AFMotor.h>

// Inicializácia čtyroch DC motorov na pozíciach 1–4
AF_DCMotor motor_1(1);
AF_DCMotor motor_2(2);
AF_DCMotor motor_3(3);
AF_DCMotor motor_4(4);

void setup() {
  Serial.begin(9600);

  // Nastav maximálnu rýchlosť všetkých motorov
  motor_1.setSpeed(255);
  motor_2.setSpeed(255);
  motor_3.setSpeed(255);
  motor_4.setSpeed(255);
}

void loop() {
  if (Serial.available()) {
    char cmd = toupper(Serial.read());
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

      case 'L':  // otáčanie doľava (ľavá zadná a predná pravá brzda)
        motor_1.run(RELEASE);
        motor_4.run(RELEASE);
        motor_2.run(FORWARD);
        motor_3.run(FORWARD);
        break;

      case 'R':  // otáčanie doprava (pravá zadná a predná ľavá brzda)
        motor_2.run(RELEASE);
        motor_3.run(RELEASE);
        motor_1.run(FORWARD);
        motor_4.run(FORWARD);
        break;

      case 'S':  // stop
        motor_1.run(RELEASE);
        motor_2.run(RELEASE);
        motor_3.run(RELEASE);
        motor_4.run(RELEASE);
        break;

      case 'M':  // meranie start – ignorovať
      case 'X':  // meranie stop – ignorovať
        // nič s motorom
        break;

      default:
        // neznámy príkaz – ignorovať
        break;
    }
  }
}
