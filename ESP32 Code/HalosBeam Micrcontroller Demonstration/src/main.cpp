// Server Address: 192.168.4.1
// Server Password: 12345678

#include <Arduino.h>
#include <Bounce2.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// LED Pins
#define mainLight 21
#define leftSignal 33
#define rightSignal 32

// Button Pins to control LEDs
#define mainButton 4
#define leftButton 2
#define rightButton 18

// Main Light Toggling
bool mainButtonState = LOW;
bool lastButtonState = LOW;
int mainLightState = 0;
unsigned long pressStartTime = 0;
unsigned long pressDuration = 0;

// Main Light States
uint8_t brightnessLvl[] = {50, 127, 255};
uint8_t brightnessIndex = 0;

// Turn signaling
bool leftIsFlashing = false;
bool leftLastButtonState = LOW;
bool leftLightState = LOW;
unsigned long leftLastFlashToggle = 0;
const unsigned long flashInterval = 400;

// Turn signaling
bool rightIsFlashing = false;
bool rightLastButtonState = LOW;
bool rightLightState = LOW;
unsigned long rightLastFlashToggle = 0;

// App Control
bool isFlashing = false;
int brightness = 255;
unsigned long lastToggleTime = 0;
const unsigned long flashIntervalApp = 500;

// Web Server configuration
const char* ssid = "ESP32-Control";
const char* password = "12345678";
AsyncWebServer server(80);

void setup() {

  // Start Serial Monitor
  Serial.begin(115200);

  // Initialize Pins for their modes
  pinMode(mainLight, OUTPUT);
  analogWrite(mainLight, mainLightState);
  pinMode(leftSignal, OUTPUT);
  digitalWrite(leftSignal, leftLightState);
  pinMode(rightSignal, OUTPUT);
  pinMode(mainButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);

  // Start Wi-Fi access point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.println(WiFi.softAPIP());

  // Serve the web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Serving main page...");
  
    String buttonText = (mainLightState > 0) ? "Turn Off Main Light" : "Turn On Main Light";
    String leftButtonText = leftIsFlashing ? "Stop Left Signal" : "Start Left Signal";
    String rightButtonText = rightIsFlashing ? "Stop Right Signal" : "Start Right Signal";
  
    // Status indicators (green = active, gray = inactive)
    String mainLightStatus = (mainLightState > 0) ? "<span class='dot green'></span>" : "<span class='dot gray'></span>";
    String leftSignalStatus = (leftIsFlashing) ? "<span class='dot green flash'></span>" : "<span class='dot gray'></span>";
    String rightSignalStatus = (rightIsFlashing) ? "<span class='dot green flash'></span>" : "<span class='dot gray'></span>";
  
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>HalosBeam Demo</title>
      <style>
        body {
          font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
          background-color: #121212;
          color: white;
          margin: 0;
          padding: 1em;
          text-align: center;
        }
        h1 {
          font-size: 2.8em;
          font-weight: 600;
          margin-top: 0.5em;
          margin-bottom: 0.2em;
        }
        h3 {
          font-size: 1.2em;
          font-weight: 400;
          color: #888;
          margin-bottom: 2em;
        }
        .button-container {
          display: flex;
          flex-direction: column;
          gap: 1.2em;
          align-items: center;
          margin-bottom: 3em;
        }
        .button-wrapper {
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 0.8em;
        }
        button {
          font-size: 1.2em;
          font-weight: 500;
          padding: 1em 2em;
          width: 90%;
          max-width: 280px;
          background-color: #1e1e1e;
          color: white;
          border: 1px solid #555;
          border-radius: 15px;
          box-shadow: 0 4px 8px rgba(0,0,0,0.4);
          transition: background-color 0.2s ease, transform 0.2s ease;
        }
        button:hover {
          background-color: #333;
          transform: scale(1.03);
        }
        .button-white {
          background-color: white;
          color: #121212;
          border: none;
        }
        .button-white:hover {
          background-color: #e0e0e0;
          transform: scale(1.03);
        }
        .dot {
          height: 18px;
          width: 18px;
          border-radius: 50%;
          display: inline-block;
        }
        .green {
          background-color: #00e676;
        }
        .gray {
          background-color: #555;
        }
        .flash {
          animation: blink 1s infinite;
        }
        @keyframes blink {
          0%, 50%, 100% { opacity: 1; }
          25%, 75% { opacity: 0.2; }
        }
        footer {
          margin-top: 3em;
          font-size: 0.8em;
          color: #777;
        }
      </style>
    </head>
    <body>
      <h1>HalosBeam</h1>
      <h3>Demonstration Circuit</h3>
  
      <div class="button-container">
        <form action="/toggle" method="get">
          <div class="button-wrapper">
            <button class="button-white" type="submit">)rawliteral" + buttonText + R"rawliteral(</button>
            )rawliteral" + mainLightStatus + R"rawliteral(
          </div>
        </form>
  
        <form action="/left" method="get">
          <div class="button-wrapper">
            <button type="submit">)rawliteral" + leftButtonText + R"rawliteral(</button>
            )rawliteral" + leftSignalStatus + R"rawliteral(
          </div>
        </form>
  
        <form action="/right" method="get">
          <div class="button-wrapper">
            <button type="submit">)rawliteral" + rightButtonText + R"rawliteral(</button>
            )rawliteral" + rightSignalStatus + R"rawliteral(
          </div>
        </form>
  
        <form action="/cycle" method="get">
          <div class="button-wrapper">
            <button type="submit">Cycle Brightness</button>
          </div>
        </form>
      </div>
  
      <footer>
        HalosBeam © 2024
      </footer>
  
    </body>
    </html>
    )rawliteral";
  
    request->send(200, "text/html", html);
  });
  
  // Toggle Main Light
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Toggle button pressed from web");
  
    if (mainLightState > 0) {
      mainLightState = 0;
    } else {
      mainLightState = 255;
    }
  
    analogWrite(mainLight, mainLightState);
    Serial.print("Main light state set to: ");
    Serial.println(mainLightState);
  
    request->redirect("/");
  });

  // Toggle Left Turn Signal
  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Left signal toggle button pressed");
    leftIsFlashing = !leftIsFlashing;
    request->redirect("/");
  });
  
  // Toggle Right Turn Signal
  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Right signal toggle button pressed");
    rightIsFlashing = !rightIsFlashing;
    request->redirect("/");
  });

  // Cycle Light Mode
  server.on("/cycle", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Brightness cycle button pressed");
  
    if (mainLightState > 0) { // Only cycle if light is ON
      analogWrite(mainLight, brightnessLvl[brightnessIndex]);
      brightnessIndex = (brightnessIndex + 1) % 3; // Cycle through 0-2
      Serial.print("Cycled to brightness level: ");
      Serial.println(brightnessLvl[brightnessIndex]);
    } else {
      Serial.println("Main light is OFF — not cycling brightness");
    }
  
    request->redirect("/");
  });

  server.begin();

}

void loop() {

  //////// BUTTON TEST ////////

  mainButtonState = digitalRead(mainButton);

  if (mainButtonState != lastButtonState) {
    delay(10); // Basic debounce
    
    mainButtonState = digitalRead(mainButton); // Recheck after debounce

    if (mainButtonState != lastButtonState) {
      lastButtonState = mainButtonState;

      if (mainButtonState == HIGH) {
        // Button pressed
        pressStartTime = millis(); // Start timer
        // Serial.println("Button pressed"); 
        
      } else { // Button released
        pressDuration = millis() - pressStartTime; // Total press time
        // Serial.print("Button released. Duration: ");
        // Serial.print(pressDuration);
        // Serial.println(" ms");
        if (pressDuration >= 1000) { // Long press (greater than 1sec)
          if (mainLightState > 0) { // If light was on, turn off
            mainLightState = 0;
          } else {
            mainLightState = 255; // If light was off, turn on
          }
          analogWrite(mainLight, mainLightState); // Toggle LED
          // Serial.println("Long press detected - LED Toggled!");
        } else {
          if (mainLightState > 0) { // Short press (greater than 0.05sec less than 1sec)
            if (pressDuration >= 50 && pressDuration < 1000) {
              analogWrite(mainLight, brightnessLvl[brightnessIndex]); // Cycle through light modes
              brightnessIndex = (brightnessIndex + 1) % 3; // Iterate through brightness levels
              // Serial.println("Brightness changed to: ");
              // Serial.println(brightnessLvl[brightnessIndex]);
            }
          }
          // Serial.println("Short press, no action!");
        }
      }
    }
  }

  bool leftButtonState = digitalRead(leftButton);

  if (leftLastButtonState == HIGH && leftButtonState == LOW) {
    leftIsFlashing = !leftIsFlashing; // Basic Debounce
  }

  leftLastButtonState = leftButtonState;

  // Turn on flashing LED
  if (leftIsFlashing) {
    if (millis() - leftLastFlashToggle >= flashInterval) {
      leftLastFlashToggle = millis();
      leftLightState = !leftLightState;
      digitalWrite(leftSignal, leftLightState);
    }
  } else {
    if (leftLightState != LOW) {
      leftLightState = LOW;
      digitalWrite(leftSignal, LOW);
    }
  }

  bool rightButtonState = digitalRead(rightButton);

  if (rightLastButtonState == HIGH && rightButtonState == LOW) {
    rightIsFlashing = !rightIsFlashing; // Basic Debounce
  }

  rightLastButtonState = rightButtonState;

  // Turn on flashing LED
  if (rightIsFlashing) {
    if (millis() - rightLastFlashToggle >= flashInterval) {
      rightLastFlashToggle = millis();
      rightLightState = !rightLightState;
      digitalWrite(rightSignal, rightLightState);
    }
  } else {
    if (rightLightState != LOW) {
      rightLightState = LOW;
      digitalWrite(rightSignal, LOW);
    }
  }

  delay(100);
  
}

