# 🐦 ESP32 Flappy Bird (1.3" OLED Edition)

A realistic, smooth 40-FPS Flappy Bird arcade game built using an **ESP32**, a **1.3-inch SH1106 I2C OLED display**, and a single **Push Button**.

## ✨ Features
- **Pixel-Art Bird Sprite:** Animated flapping wings and nose-dive physics.
- **Crash Animation:** Screen flash on impact followed by a falling animation.
- **3D-Styled Pipes & Scrolling Ground:** Shaded Mario-style pipes and moving grass.
- **Live Score & High Score Popup:** Clean centered scoreboard after Game Over.

## 🔌 Pin Connections

| Component Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **OLED VCC** | 3.3V | Power |
| **OLED GND** | GND | Common Ground |
| **OLED SCL** | GPIO 22 | Hardware I2C Clock |
| **OLED SDA** | GPIO 21 | Hardware I2C Data |
| **Button (Leg 1)** | GPIO 4 | Uses internal `INPUT_PULLUP` |
| **Button (Leg 2)** | GND | Triggers on LOW |

## 📚 Required Libraries
Install these via the Arduino IDE Library Manager:
1. `Adafruit SH110X`
2. `Adafruit GFX Library`

## 🚀 How to Run
1. Wire the components according to the table above.
2. Open `FlappyBird_ESP32.ino` in Arduino IDE.
3. Select your ESP32 board and COM port, then click **Upload**.
