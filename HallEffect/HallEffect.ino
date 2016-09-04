const int led = 13;
const int sensor = 7;
const int vcc = 6;
const int ground = 5;

void setup() {
  //start serial connection
  delay(5000);
  Serial.begin(115200);
  Serial.println("Running...");
  pinMode(led, OUTPUT); digitalWrite(led, LOW);

}

void loop() {
  static int previous = HIGH;

  //read the sensor value into a variable
  int sensorVal = readHall();
  //print out the value of the sensor
  //Serial.println(sensorVal);

  if (sensorVal == HIGH) {
    digitalWrite(led, LOW);
  } else {
    digitalWrite(led, HIGH);
  }
  
  if (sensorVal != previous) {
    Serial.print("Sensor changed to "); Serial.println(sensorVal);
  }
  previous = sensorVal;
}

int readHall(void) {
  int value;
  
  // Power up sensor
  pinMode(vcc, OUTPUT); digitalWrite(vcc, HIGH);
  pinMode(ground, OUTPUT); digitalWrite(ground, LOW);
  pinMode(sensor, INPUT_PULLUP);

  // Wait to settle
  delay(100);

  // Read
  value = digitalRead(sensor);

  // Power down (set pins to high impedance)
  pinMode(vcc, INPUT); pinMode(ground, INPUT); pinMode(sensor, INPUT);

  return value;
}

