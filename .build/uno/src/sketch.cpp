#include <Arduino.h>
#include <EEPROM.h>
void setup();
void loop();
void setClamp(bool clamping);
void setPump(bool pumping);
void setLED(bool on);
float readDiodePin();
float measureOD();
void recordData(float fval);
void initDataLog();
void outputDataLog();
#line 1 "src/sketch.ino"
//#include <EEPROM.h>

// Hardcoded pins on the Arduino Nano
// Analog Pins
const int diodePin = 1;
// Digital Pins
const int ledPin = 5;
const int airflowPin = 13; //const int clampPin = 3; // I renamed this since we moved from a pinch valve to a solid state relay to control airflow
const int pumpPin = 2;

// Constants
const unsigned char MAX_PUMP_SPEED = 120; // Speed ranges from 0 to 200 Hz
    
// Time in millis
const unsigned int MILLISECOND = 1;
const unsigned int SECOND = 1000 * MILLISECOND;
const unsigned int MINUTE = 60 * SECOND;

// What OD we are aiming for
const float OD_TARGET = 0.4;

// In-memory storage space
unsigned int usedLogs = 0;
const unsigned int NUM_OD_LOGS = 512;

unsigned int iteration = 0;

// Enable self test?
byte SELF_TEST = 1;

//Enable initial filling routine?
byte INIT_FILL = 0;

void setup() {  
  // Output EEPROM values.
  outputDataLog();
  
  // Set pin modes
  pinMode(ledPin, OUTPUT);
  pinMode(airflowPin, OUTPUT);
  setClamp(false); // allow the airflow pump to run
  pinMode(diodePin, INPUT);
  
  // Self test
  if(SELF_TEST) {
    Serial.begin(9600);
    Serial.println("Waiting for test...");
    delay(5 * SECOND);
    Serial.println("Starting test.");
    Serial.println("Turning pump on.");
    setPump(true);
    delay(5 * SECOND);
    Serial.println("Turning pump off.");
    setPump(false);
    delay(5 * SECOND);
    Serial.println("Turning clamp on.");
    setClamp(true);
    delay(5 * SECOND);
    Serial.println("Turning clamp off.");
    setClamp(false);
    delay(5 * SECOND);
    Serial.println("Testing LED and OD sensor.");
    for(unsigned int i = 0; i < 3; i++) {
      Serial.println(measureOD()); 
    }
    Serial.println("Done with testing.");
    Serial.end();
  }
 
  if (INIT_FILL) { 
  	// Fill the growth flask
	  setPump(true);
	  for(unsigned int i = 0; i < 10; i++) {
		  delay(MINUTE); 
          }
  }
  
  // No pump
  setPump(false);
  
  // Have air on always
  setClamp(false);
}

void loop() {
    iteration++;
    
    float od = measureOD();
    
    // Output value
    Serial.begin(9600);
    Serial.print("Measured: ");
    Serial.println(od);
    Serial.end();
    if(iteration % 5 == 0) {
      recordData(od);
    }
    
    if(od > OD_TARGET) {
        setPump(true); 
    } else {
        setPump(false); 
    }
        

    delay(MINUTE);
}

void setClamp(bool clamping) {
 digitalWrite(airflowPin, clamping ? LOW : HIGH); //digitalWrite(clampPin, clamping ? HIGH : LOW); 
 //clamping true turns off the air. clamping false turns it on.
}

void setPump(bool pumping) {
  if(pumping) {
    tone(pumpPin, MAX_PUMP_SPEED);
  } else {
    noTone(pumpPin); 
  }
}

void setLED(bool on) {
    digitalWrite(ledPin, on ? HIGH : LOW);
}

float readDiodePin() {
    // Turn LED pin on
    setLED(true);
    
   // Read the pin a bunch of times
   unsigned long val = 0;
   const unsigned int numMeasurements = 1000;
   for(unsigned int i = 0; i < numMeasurements; i++) {
       val += analogRead(diodePin);
       delay(MILLISECOND);
   }
   
    // Turn LED pin off
    setLED(false);
   
   // Average the value from the pin
   float average = (float) (val) / (float) (numMeasurements);
   return average;
}

float measureOD() {
    // Bubbles interfere with measurement
    setClamp(true);
    delay(5 * SECOND);
    
    float diodeValue = readDiodePin();
    
    // Allow air to flow again
    setClamp(false);
    
    // Exponential coefficients: 779.02987726, -2.42854577112 
    // y = 779 * exp(-2.4 x)
    // Take the inverse
    // return log(diodeValue / 779) / -2.4;

    // new calibration was done by Jenn and Jacob on Mar 24 2016
    return log(diodeValue / 757.9) / -1.841;
}

// Record a data point in the global data log.
// If there's no space, ditch the oldest value.
void recordData(float fval) {
  byte value = fval <= 0 ? 0 : (byte) (fval * 100);
  
  // If we have more space, just stick the value
  // at the back of the data log.
  if(usedLogs < NUM_OD_LOGS) {
    EEPROM.write(usedLogs, value);
    usedLogs++;
  } else {
    // If we have no more space, move all values over to discard oldest,
    // then insert the new value at the very end of the data log.
    
    // Move everything over
    for(unsigned int i = 0; i < NUM_OD_LOGS - 1; i++) {
      EEPROM.write(i, EEPROM.read(i + 1)); 
    }
    
    // Add new value at the end
    EEPROM.write(NUM_OD_LOGS - 1, value);
  }
}

// Set data log to zeros
void initDataLog() {
  for(unsigned int i = 0; i < NUM_OD_LOGS; i++) {
    EEPROM.write(i, 0);
  } 
}

// Output data log to serial monitor
void outputDataLog() {
  Serial.begin(9600);
  for(unsigned int i = 0; i < NUM_OD_LOGS; i++) {
    if(i != 0) {
      Serial.print(",");
    }
    Serial.print(EEPROM.read(i));
  }
  Serial.println();
  Serial.end();
}

