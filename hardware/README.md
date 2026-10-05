# Hardware Documentation

## 1. Main Controller

### ESP32-CAM

The ESP32-CAM is used as the main embedded controller and camera platform.

Functions:

- Camera capture
- Wi-Fi communication
- Robot control
- Sensor monitoring
- Communication with the AI system

---

## 2. Motor System

### DC Geared Motors

Two DC geared motors are used to move the robot.

The motors provide:

- Forward movement
- Reverse movement
- Left turning
- Right turning

### Motor Driver

A motor driver is used between the ESP32-CAM and the DC motors because the ESP32-CAM cannot directly drive the motors.

---

## 3. IR Obstacle Sensors

Two IR obstacle sensors are used:

- Left IR sensor
- Right IR sensor

They help detect nearby obstacles.

The sensor information is used by the robot's obstacle-avoidance system.

---

## 4. Ultrasonic Sensor

An ultrasonic sensor is used to measure the distance between the robot and an obstacle.

The distance is calculated from the ultrasonic echo time.

If an obstacle is detected within the predefined safety distance, the robot can stop or change direction.

---

## 5. LCD Display

A 16x2 I2C LCD is used to display robot and safety information.

Example messages:

- ROBOT READY
- OBSTACLE DETECTED
- SAFETY NORMAL
- NO HELMET
- NO BARRICADE
- ROBOT STOPPED

---

## 6. Camera and AI

The ESP32-CAM captures images of the construction environment.

The AI system can analyse the images for:

### Helmet Detection

Determines whether workers are wearing safety helmets.

### Barricade Detection

Identifies safety barricades in the monitored area.

The detection result is communicated to the robot controller.

---

## 7. Power Supply

A battery supply is used to power the robot.

The motor supply and controller supply must be appropriately regulated.

The motors should not be powered directly from an ESP32-CAM GPIO pin.

---

# System Block Diagram

```text
                    ESP32-CAM
                        |
          +-------------+-------------+
          |             |             |
       Camera        Sensors       Wi-Fi
          |             |             |
          |       +-----+-----+       |
          |       |           |       |
          |      IR       Ultrasonic  |
          |       |           |       |
          |       +-----+-----+       |
          |             |             |
          |        Robot Control      |
          |             |             |
          |        Motor Driver       |
          |             |             |
          |        DC Motors          |
          |
          v
     AI Detection
          |
     +----+-----+
     |          |
  Helmet     Barricade
 Detection   Detection
     |          |
     +----+-----+
          |
     Safety Status
          |
       I2C LCD
