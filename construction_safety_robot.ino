#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// WIFI
// ============================================================

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// ============================================================
// LCD
// ============================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================================
// ESP32-CAM AI THINKER CAMERA PINS
// ============================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================
// MOTOR DRIVER
// ============================================================

#define LEFT_IN1   12
#define LEFT_IN2   13

#define RIGHT_IN1  14
#define RIGHT_IN2  15

// ============================================================
// IR OBSTACLE SENSORS
// ============================================================

// Change these according to your actual wiring.

#define IR_LEFT   1
#define IR_RIGHT  3

// Most IR obstacle modules output LOW when an obstacle
// is detected.

#define OBSTACLE_DETECTED LOW

// ============================================================
// ULTRASONIC SENSOR
// ============================================================

#define TRIG_PIN 4
#define ECHO_PIN 16

const int MIN_DISTANCE = 20;   // cm

// ============================================================
// SAFETY STATUS
// ============================================================

bool helmetDetected = false;
bool barricadeDetected = false;

String safetyStatus = "NORMAL";

bool automaticMode = true;

// ============================================================
// LCD FUNCTION
// ============================================================

void showLCD(String line1, String line2)
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));

  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

// ============================================================
// MOTOR STOP
// ============================================================

void stopMotors()
{
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);

  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);
}

// ============================================================
// FORWARD
// ============================================================

void moveForward()
{
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);

  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);
}

// ============================================================
// BACKWARD
// ============================================================

void moveBackward()
{
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, HIGH);

  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, HIGH);
}

// ============================================================
// LEFT
// ============================================================

void turnLeft()
{
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, HIGH);

  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);
}

// ============================================================
// RIGHT
// ============================================================

void turnRight()
{
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);

  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, HIGH);
}

// ============================================================
// ULTRASONIC DISTANCE
// ============================================================

long getDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
  {
    return 999;
  }

  long distance = duration * 0.0343 / 2;

  return distance;
}

// ============================================================
// IR SENSOR CHECK
// ============================================================

bool leftObstacle()
{
  return digitalRead(IR_LEFT) == OBSTACLE_DETECTED;
}

bool rightObstacle()
{
  return digitalRead(IR_RIGHT) == OBSTACLE_DETECTED;
}

// ============================================================
// OBSTACLE AVOIDANCE
// ============================================================

void automaticObstacleAvoidance()
{
  bool leftBlocked = leftObstacle();
  bool rightBlocked = rightObstacle();

  long distance = getDistance();

  if (distance < MIN_DISTANCE)
  {
    stopMotors();

    showLCD("OBSTACLE!", "DIST: " + String(distance) + "cm");

    delay(300);

    if (!rightBlocked)
    {
      turnRight();
      delay(500);
      stopMotors();
    }
    else if (!leftBlocked)
    {
      turnLeft();
      delay(500);
      stopMotors();
    }
    else
    {
      moveBackward();
      delay(500);
      stopMotors();
    }

    return;
  }

  if (leftBlocked && rightBlocked)
  {
    stopMotors();
    showLCD("OBSTACLE!", "LEFT + RIGHT");
    return;
  }

  if (leftBlocked)
  {
    turnRight();
    showLCD("OBSTACLE LEFT", "TURNING RIGHT");
    return;
  }

  if (rightBlocked)
  {
    turnLeft();
    showLCD("OBSTACLE RIGHT", "TURNING LEFT");
    return;
  }
}

// ============================================================
// SAFETY DECISION
// ============================================================

void updateSafetyStatus()
{
  if (!helmetDetected)
  {
    safetyStatus = "NO HELMET";
  }
  else if (!barricadeDetected)
  {
    safetyStatus = "NO BARRICADE";
  }
  else
  {
    safetyStatus = "NORMAL";
  }

  showLCD("SAFETY STATUS", safetyStatus);
}

// ============================================================
// AI SERIAL COMMAND
// ============================================================
//
// The external AI system can send:
//
// HELMET:1
// HELMET:0
// BARRICADE:1
// BARRICADE:0
//
// Example:
//
// HELMET:1
// BARRICADE:1
//
// ============================================================

void processAICommand(String command)
{
  command.trim();

  if (command == "HELMET:1")
  {
    helmetDetected = true;
  }

  else if (command == "HELMET:0")
  {
    helmetDetected = false;
  }

  else if (command == "BARRICADE:1")
  {
    barricadeDetected = true;
  }

  else if (command == "BARRICADE:0")
  {
    barricadeDetected = false;
  }

  updateSafetyStatus();

  Serial.print("Helmet: ");
  Serial.println(helmetDetected ? "DETECTED" : "NOT DETECTED");

  Serial.print("Barricade: ");
  Serial.println(barricadeDetected ? "DETECTED" : "NOT DETECTED");

  Serial.print("Safety: ");
  Serial.println(safetyStatus);
}

// ============================================================
// ROOT WEB PAGE
// ============================================================

void handleRoot()
{
  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>
Construction Safety Robot
</title>

<style>

body
{
    background:#111;
    color:white;
    font-family:Arial;
    text-align:center;
}

h1
{
    color:#ffcc00;
}

button
{
    width:120px;
    height:60px;
    margin:8px;
    font-size:16px;
    font-weight:bold;
    border-radius:10px;
    border:none;
}

.forward
{
    background:#2ecc71;
}

.backward
{
    background:#e74c3c;
}

.left
{
    background:#3498db;
}

.right
{
    background:#3498db;
}

.stop
{
    background:#f1c40f;
}

</style>

<script>

function command(cmd)
{
    fetch("/" + cmd);
}

</script>

</head>

<body>

<h1>
Construction Safety Robot
</h1>

<p>
Remote Robot Control
</p>

<button class="forward"
onclick="command('forward')">
FORWARD
</button>

<br>

<button class="left"
onclick="command('left')">
LEFT
</button>

<button class="stop"
onclick="command('stop')">
STOP
</button>

<button class="right"
onclick="command('right')">
RIGHT
</button>

<br>

<button class="backward"
onclick="command('backward')">
BACKWARD
</button>

<h2>
Safety Monitoring
</h2>

<p>
Helmet Detection
</p>

<p>
Barricade Detection
</p>

</body>

</html>

)rawliteral";

  server.send(200, "text/html", html);
}

// ============================================================
// WEB COMMANDS
// ============================================================

void handleForward()
{
  moveForward();

  server.send(
    200,
    "text/plain",
    "Moving Forward"
  );
}

void handleBackward()
{
  moveBackward();

  server.send(
    200,
    "text/plain",
    "Moving Backward"
  );
}

void handleLeft()
{
  turnLeft();

  server.send(
    200,
    "text/plain",
    "Turning Left"
  );
}

void handleRight()
{
  turnRight();

  server.send(
    200,
    "text/plain",
    "Turning Right"
  );
}

void handleStop()
{
  stopMotors();

  server.send(
    200,
    "text/plain",
    "Robot Stopped"
  );
}

// ============================================================
// CAMERA INITIALIZATION
// ============================================================

void startCamera()
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t error =
      esp_camera_init(&config);

  if (error != ESP_OK)
  {
    Serial.print(
      "Camera Error: 0x"
    );

    Serial.println(
      error,
      HEX
    );

    return;
  }

  Serial.println(
    "Camera initialized."
  );
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  // ---------------- MOTOR ----------------

  pinMode(
    LEFT_IN1,
    OUTPUT
  );

  pinMode(
    LEFT_IN2,
    OUTPUT
  );

  pinMode(
    RIGHT_IN1,
    OUTPUT
  );

  pinMode(
    RIGHT_IN2,
    OUTPUT
  );

  stopMotors();

  // ---------------- IR ----------------

  pinMode(
    IR_LEFT,
    INPUT
  );

  pinMode(
    IR_RIGHT,
    INPUT
  );

  // ---------------- ULTRASONIC ----------------

  pinMode(
    TRIG_PIN,
    OUTPUT
  );

  pinMode(
    ECHO_PIN,
    INPUT
  );

  // ---------------- LCD ----------------

  Wire.begin();

  lcd.init();

  lcd.backlight();

  showLCD(
    "Safety Robot",
    "Starting..."
  );

  delay(1500);

  // ---------------- CAMERA ----------------

  startCamera();

  // ---------------- WIFI ----------------

  WiFi.begin(
    ssid,
    password
  );

  showLCD(
    "Connecting WiFi",
    "Please wait..."
  );

  Serial.print(
    "Connecting to WiFi"
  );

  while (
    WiFi.status() != WL_CONNECTED
  )
  {
    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "WiFi connected"
  );

  Serial.print(
    "IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );

  showLCD(
    "WiFi Connected",
    WiFi.localIP().toString()
  );

  // ---------------- WEB SERVER ----------------

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/forward",
    handleForward
  );

  server.on(
    "/backward",
    handleBackward
  );

  server.on(
    "/left",
    handleLeft
  );

  server.on(
    "/right",
    handleRight
  );

  server.on(
    "/stop",
    handleStop
  );

  server.begin();

  Serial.println(
    "Robot server started."
  );

  showLCD(
    "Robot Ready",
    "Safety Mode"
  );
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  server.handleClient();

  // ---------------------------------------------
  // Read AI commands from serial
  // ---------------------------------------------

  if (Serial.available())
  {
    String command =
      Serial.readStringUntil('\n');

    processAICommand(command);
  }

  // ---------------------------------------------
  // Automatic obstacle avoidance
  // ---------------------------------------------

  if (automaticMode)
  {
    automaticObstacleAvoidance();
  }

  delay(50);
}
