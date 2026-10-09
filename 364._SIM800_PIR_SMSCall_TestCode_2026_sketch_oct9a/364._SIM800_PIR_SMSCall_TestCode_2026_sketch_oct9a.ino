#include <SoftwareSerial.h>

#define rxPin 16
#define txPin 17
SoftwareSerial sim800L(rxPin, txPin); 

#define pirPin 5
#define buzzerPin 18

bool pirEnabled = true; // Software switch variable to enable/disable PIR
String buff;

// Variables for non-blocking PIR debounce
unsigned long pirHighStartTime = 0;
bool motionStateRecorded = false;
const unsigned long debounceTime = 100; // Must stay HIGH for at least 100ms

void setup() {
  Serial.begin(115200);
  delay(500);
  
  sim800L.begin(9600);
  delay(500);

  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.println("Initializing...");
  
  sim800L.println("AT");
  waitForResponse();

  sim800L.println("ATE1");
  waitForResponse();

  sim800L.println("AT+CMGF=1");
  waitForResponse();

  sim800L.println("AT+CNMI=1,2,0,0,0");
  waitForResponse();

  Serial.println("System Ready. Commands: 's' for SMS, 'c' for Call, 'pe' to enable PIR, 'pd' to disable PIR.");
}

void loop() {
  // Non-blocking PIR noise elimination using millis()
  if (pirEnabled) {
    if (digitalRead(pirPin) == HIGH) {
      if (pirHighStartTime == 0) {
        pirHighStartTime = millis(); // Mark the moment pin goes HIGH
      } else if (!motionStateRecorded && (millis() - pirHighStartTime >= debounceTime)) {
        // If it has stayed HIGH continuously for 100ms, consider it valid motion
        Serial.println("Motion detected by PIR sensor!");
        triggerAlarmSequence();
        motionStateRecorded = true; // Prevent re-triggering immediately
      }
    } else {
      // Reset when pin goes LOW again
      pirHighStartTime = 0;
      motionStateRecorded = false;
    }
  }

  while (sim800L.available()) {
    buff = sim800L.readString();
    Serial.println(buff);
  }
  while (Serial.available()) {
    buff = Serial.readString();
    buff.trim();
    if (buff == "s") {
      send_sms();
    }
    else if (buff == "c") {
      make_call();
    }
    else if (buff == "pe") {
      pirEnabled = true;
      Serial.println("PIR Sensor Enabled.");
    }
    else if (buff == "pd") {
      pirEnabled = false;
      Serial.println("PIR Sensor Disabled.");
    }
    else {
      sim800L.println(buff);
    }
  }
  delay(10); // Short delay for loop stability
}

void triggerAlarmSequence() {
  // Sound buzzer in an alarm tone pattern for 5 seconds
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    tone(buzzerPin, 2000);
    delay(200);
    tone(buzzerPin, 1000);
    delay(200);
  }
  noTone(buzzerPin);

  // Send SMS and make call
  send_sms();
  delay(5000);
  make_call();
}

void send_sms() {
  sim800L.print("AT+CMGS=\"+918860055120\"\r");
  waitForResponse();
  
  sim800L.print("Alert! Motion detected by PIR sensor.");
  
  sim800L.write(0x1A); // End of message (Ctrl+Z)
  waitForResponse();
}

void make_call() {
  sim800L.println("ATD+918860055120;");
  waitForResponse();
}

void waitForResponse() {
  delay(2000);
  while (sim800L.available()) {
    Serial.println(sim800L.readString());
  }
  sim800L.read();
}