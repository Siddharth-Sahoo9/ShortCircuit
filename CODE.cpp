// Pin definitions for traffic lights and button
const int RED_PIN = 2;
const int YELLOW_PIN = 3;
const int GREEN_PIN = 4;
const int BUTTON_PIN = 7;

// Pin definitions for ultrasonic sensor
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

// Timing constants (in milliseconds)
const unsigned long GREEN_DURATION = 5000;
const unsigned long YELLOW_DURATION = 2000;
const unsigned long RED_DURATION = 5000;
const unsigned long PEDESTRIAN_CROSSING_TIME = 8000;
const unsigned long VEHICLE_CHECK_INTERVAL = 1000;  // Check for vehicles every second
const unsigned long NO_VEHICLE_TIMEOUT = 30000;     // 30 seconds without vehicle

// Distance threshold for vehicle detection (in cm)
const int VEHICLE_DISTANCE_THRESHOLD = 100;

// Variables to track state
enum TrafficState {
  GREEN_LIGHT,
  YELLOW_LIGHT,
  RED_LIGHT,
  POWER_SAVING_RED
};

TrafficState currentState = GREEN_LIGHT;
unsigned long stateStartTime = 0;
unsigned long lastVehicleCheckTime = 0;    // Time of last vehicle check
unsigned long noVehicleStartTime = 0;      // Time when we first detected no vehicle
bool pedestrianRequested = false;
bool vehiclePresent = false;
bool isCountingNoVehicle = false;          // Flag to track if we're counting the 30 seconds

// Function to measure distance using ultrasonic sensor
int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

// Function to check for vehicle presence
bool checkForVehicle() {
  int distance = measureDistance();
  return distance < VEHICLE_DISTANCE_THRESHOLD;
}

// Function to switch to red light (for both normal and power saving modes)
void switchToRed(bool isPowerSaving = false) {
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);
  currentState = isPowerSaving ? POWER_SAVING_RED : RED_LIGHT;
  stateStartTime = millis();
}

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Start with green light
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  
  stateStartTime = millis();
  lastVehicleCheckTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check for pedestrian button press
  if (digitalRead(BUTTON_PIN) == HIGH && !pedestrianRequested) {
    if (currentState == POWER_SAVING_RED) {
      // If in power saving, just start pedestrian crossing timer
      currentState = RED_LIGHT;
      stateStartTime = currentTime;
      pedestrianRequested = true;
    } else if (currentState != RED_LIGHT) {
      // For other states, switch to red
      switchToRed();
      pedestrianRequested = true;
    }
  }
  
  // Check for vehicle presence at regular intervals
  if (currentTime - lastVehicleCheckTime >= VEHICLE_CHECK_INTERVAL) {
    lastVehicleCheckTime = currentTime;
    vehiclePresent = checkForVehicle();
    
    if (vehiclePresent) {
      // Reset the no-vehicle timer when a vehicle is detected
      isCountingNoVehicle = false;
      
      // If in power saving, resume normal cycle with green light
      if (currentState == POWER_SAVING_RED) {
        digitalWrite(RED_PIN, LOW);
        digitalWrite(GREEN_PIN, HIGH);
        currentState = GREEN_LIGHT;
        stateStartTime = currentTime;
      }
    } 
    else {  // No vehicle detected
      if (!isCountingNoVehicle) {
        // Start the 30-second timer when no vehicle is first detected
        noVehicleStartTime = currentTime;
        isCountingNoVehicle = true;
      }
      // Check if 30 seconds have passed with no vehicle
      else if ((currentTime - noVehicleStartTime >= NO_VEHICLE_TIMEOUT) && 
               (currentState != POWER_SAVING_RED) && 
               (currentState != RED_LIGHT)) {
        // Only switch to power saving red after 30 seconds of no vehicles
        switchToRed(true);
      }
    }
  }
  
  // Skip normal cycle if in power saving mode
  if (currentState == POWER_SAVING_RED) {
    return;
  }
  
  unsigned long stateElapsedTime = currentTime - stateStartTime;
  
  // Handle normal traffic light cycle
  switch (currentState) {
    case GREEN_LIGHT:
      if (stateElapsedTime >= GREEN_DURATION) {
        digitalWrite(GREEN_PIN, LOW);
        digitalWrite(YELLOW_PIN, HIGH);
        currentState = YELLOW_LIGHT;
        stateStartTime = currentTime;
      }
      break;
      
    case YELLOW_LIGHT:
      if (stateElapsedTime >= YELLOW_DURATION) {
        switchToRed();
      }
      break;
      
    case RED_LIGHT:
      if (stateElapsedTime >= (pedestrianRequested ? PEDESTRIAN_CROSSING_TIME : RED_DURATION)) {
        if (vehiclePresent || pedestrianRequested) {
          digitalWrite(RED_PIN, LOW);
          digitalWrite(GREEN_PIN, HIGH);
          currentState = GREEN_LIGHT;
          stateStartTime = currentTime;
          pedestrianRequested = false;
        } 
        else {
          // Check if we've passed the 30-second no-vehicle threshold
          if (isCountingNoVehicle && (currentTime - noVehicleStartTime >= NO_VEHICLE_TIMEOUT)) {
            switchToRed(true);  // Switch to power saving red
          } else {
            // Continue normal cycle if 30 seconds haven't passed
            digitalWrite(RED_PIN, LOW);
            digitalWrite(GREEN_PIN, HIGH);
            currentState = GREEN_LIGHT;
            stateStartTime = currentTime;
          }
        }
      }
      break;
  }
}