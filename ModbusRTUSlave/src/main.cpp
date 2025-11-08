#include <Arduino.h>
#include <ModbusRTUSlave.h>

// Modbus RTU slave on default Serial port
ModbusRTUSlave modbus(Serial);

// Modbus register arrays
uint16_t holdingRegs[2];       // Writable registers (e.g., setpoints or control values)
uint16_t inputRegs[2];         // Read-only registers (e.g., sensor feedback)
bool coils[2];                 // Writable bits (e.g., digital outputs like LEDs)
bool discreteInputs[4];        // Read-only bits (e.g., buttons or switches)

// Structs for organizing I/O data
struct AnalogPins {
  int analog[2];               // Stores analog sensor readings
};

struct DigitalPins {
  bool digital[4];             // Stores digital input states (HIGH/LOW)
};

struct DigitalOutputs {
  int digital[2];              // Stores output pin numbers (e.g., LED pins)
};

// Function declarations
DigitalOutputs writeDigital(int digitalOut0, int digitalOut1);     // Maps output pins
DigitalPins readDigital(const int inputPins[4]);                   // Reads digital inputs
AnalogPins readAnalog(const int analogPins[2]);                    // Reads analog inputs

// Pin mappings
const int analogInputPins[2] = {A0, A1};                           // Analog sensors
const int digitalInputPins[4] = {2, 3, 4, 5};                      // Digital inputs
DigitalOutputs digitalOutput;                                      // Output pin container

void setup() {
  Serial.begin(9600);                                              // Initialize serial communication
  modbus.begin(1, 9600, SERIAL_8N1);                               // Modbus slave ID = 1, baud = 9600

  // Configure Modbus register arrays
  modbus.configureHoldingRegisters(holdingRegs, 2);
  modbus.configureInputRegisters(inputRegs, 2);
  modbus.configureCoils(coils, 2);
  modbus.configureDiscreteInputs(discreteInputs, 4);

  // Define output pins (e.g., LEDs)
  digitalOutput = writeDigital(6, 7);

  // Initialize output and analog input pins
  for (int i = 0; i < 2; i++) {
    pinMode(digitalOutput.digital[i], OUTPUT);                    // Set output pins
    pinMode(analogInputPins[i], INPUT);                           // Set analog pins
    coils[i] = false;                                             // Initialize coil states to LOW
  }

  // Initialize digital input pins with internal pull-ups
  for (int i = 0; i < 4; i++) {
    pinMode(digitalInputPins[i], INPUT_PULLUP);                   // Prevent floating inputs
  }
}

void loop() {
  modbus.poll();                                                  // Handle Modbus communication

  // Refresh sensor readings
  AnalogPins analogInput = readAnalog(analogInputPins);
  DigitalPins digitalInput = readDigital(digitalInputPins);

  // Update Modbus registers and control outputs
  for (int i = 0; i < 2; i++) {
    inputRegs[i] = analogInput.analog[i];                         // Reflect analog sensor values
    holdingRegs[i] = analogInput.analog[i];                       // Mirror to holding registers
    digitalWrite(digitalOutput.digital[i], coils[i]);             // Set output pins based on coil states
  }

  // Update discrete input states
  for (int i = 0; i < 4; i++) {
    discreteInputs[i] = digitalInput.digital[i];                  // Reflect button/switch states
  }
}

// Maps output pin numbers into a struct
DigitalOutputs writeDigital(int digitalOut0, int digitalOut1) {
  DigitalOutputs pins;
  int values[2] = {digitalOut0, digitalOut1};
  for (int i = 0; i < 2; i++) {
    pins.digital[i] = values[i];
  }
  return pins;
}

// Reads analog sensor values
AnalogPins readAnalog(const int analogPins[2]) {
  AnalogPins pins;
  for (int i = 0; i < 2; i++) {
    pins.analog[i] = analogRead(analogPins[i]);
  }
  return pins;
}

// Reads digital input states (buttons, switches)
DigitalPins readDigital(const int inputPins[4]) {
  DigitalPins pins;
  for (int i = 0; i < 4; i++) {
    pins.digital[i] = digitalRead(inputPins[i]); // HIGH = unpressed, LOW = pressed
  }
  return pins;
}

