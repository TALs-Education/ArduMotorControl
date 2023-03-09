#include <ArduinoJson.h>

const int SENSOR_1_PIN = A0;
const int SENSOR_2_PIN = A1;
const int SENSOR_3_PIN = A2;
const int UPDATE_INTERVAL_MS = 100;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void setup() {
  Serial.begin(115200);
  while (!Serial) continue; // Wait for serial connection
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}

void loop() {
  static unsigned long last_update_time = 0;

  // Check if it's time to update sensor readings
  unsigned long current_time = millis();
  if (current_time - last_update_time >= UPDATE_INTERVAL_MS) {
      last_update_time = current_time;

      // Read sensor values
      int sensor1_value = analogRead(SENSOR_1_PIN);
      //int sensor2_value = analogRead(SENSOR_2_PIN);
      //int sensor3_value = analogRead(SENSOR_3_PIN);
    
      // Create JSON message
      StaticJsonDocument<64> doc;
      //doc["time"] = current_time;
      doc["sensor1"] = sensor1_value;
      //doc["sensor2"] = sensor2_value;
      //doc["sensor3"] = sensor3_value;
      serializeJson(doc, Serial);
      Serial.println(); // Add a newline character to signal end of message   
  }

  // Parse incoming string
  if (stringComplete) {
    StaticJsonDocument<64> incoming_doc;
    DeserializationError error = deserializeJson(incoming_doc, inputString);
    if (!error) {
      // Process incoming message
      if (incoming_doc.containsKey("command")) {
        String command = incoming_doc["command"].as<String>();
        if (command == "reset") {
          Serial.println("{'reset': 0}"); // Add a newline character to signal end of message  
        }
      }
    }
    inputString = "";
    stringComplete = false;
  } 
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
