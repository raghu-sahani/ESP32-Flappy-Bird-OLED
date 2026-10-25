#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 4

// Game states (Added DYING state for crash animation)
enum GameState { START_SCREEN, PLAYING, DYING, GAME_OVER };
GameState state = START_SCREEN;

// Bird physics & dimensions (12x8 pixel realistic sprite)
const int birdX = 26;
const int birdW = 12;
const int birdH = 8;
float birdY = 28.0;
float birdVelocity = 0.0;
const float gravity = 0.26;
const float jumpForce = -2.5;

// Ground settings
const int groundY = 58; // Ground starts at Y=58 (6 pixels tall)
int groundOffset = 0;

// Pipe variables
float pipeX = SCREEN_WIDTH;
int pipeGapY = 20;
const int pipeWidth = 14;
const int pipeGapHeight = 23; // Gap size for bird
float pipeSpeed = 1.6;

// Score & Timing
int score = 0;
int highScore = 0;
bool passedPipe = false;
unsigned long stateTimer = 0;

bool lastButtonState = HIGH;
unsigned long lastFrameTime = 0;
const int frameDelay = 25; // ~40 FPS

// 12x8 Pixel Art Bird (Wing Up)
const unsigned char PROGMEM birdWingUp[] = {
  0x07, 0x80, // .....####.......
  0x18, 0x40, // ...##....#......
  0x27, 0x20, // ..#..###..#.....
  0x78, 0xA0, // .####...#.#.....
  0x80, 0xFE, // #.......#######.
  0x40, 0x82, // .#......#.....#.
  0x3F, 0xFC, // ..############..
  0x0F, 0x00  // ....####........
};

// 12x8 Pixel Art Bird (Wing Down)
const unsigned char PROGMEM birdWingDown[] = {
  0x07, 0x80, // .....####.......
  0x18, 0x40, // ...##....#......
  0x20, 0x20, // ..#.......#.....
  0x40, 0xA0, // .#......#.#.....
  0x78, 0xFE, // .####...#######.
  0x84, 0x82, // #...#...#.....#.
  0x7F, 0xFC, // .#############..
  0x0F, 0x00  // ....####........
};

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(34));

  delay(250);
  if (!display.begin(I2C_ADDRESS, true)) {
    while (1);
  }
  display.clearDisplay();
  display.display();
}

void resetGame() {
  birdY = 26;
  birdVelocity = 0;
  pipeX = SCREEN_WIDTH;
  pipeGapY = random(8, groundY - pipeGapHeight - 8);
  score = 0;
  pipeSpeed = 1.6;
  passedPipe = false;
}

bool isButtonJustPressed() {
  bool currentState = digitalRead(BUTTON_PIN);
  bool pressed = (currentState == LOW && lastButtonState == HIGH);
  lastButtonState = currentState;
  return pressed;
}

// Draws the bird based on vertical speed
void drawBird(int x, int y) {
  if (state == DYING || state == GAME_OVER) {
    // Nose-diving bird when crashed
    display.drawBitmap(x, y, birdWingDown, 12, 8, SH110X_WHITE);
  } else if (birdVelocity < 0) {
    // Flapping up
    display.drawBitmap(x, y, birdWingDown, 12, 8, SH110X_WHITE);
  } else {
    // Gliding down
    display.drawBitmap(x, y, birdWingUp, 12, 8, SH110X_WHITE);
  }
}

// Draws 3D-styled pipes with rims and inner shading
void drawPipes(int x, int gapY) {
  int bottomY = gapY + pipeGapHeight;

  // --- TOP PIPE ---
  if (gapY > 4) {
    display.drawRect(x, 0, pipeWidth, gapY - 3, SH110X_WHITE);
    display.drawFastVLine(x + 2, 0, gapY - 3, SH110X_WHITE); // 3D Highlight line
  }
  // Top Pipe Lip (Rim)
  display.fillRect(x - 1, gapY - 4, pipeWidth + 2, 4, SH110X_WHITE);
  display.drawRect(x - 1, gapY - 4, pipeWidth + 2, 4, SH110X_BLACK); // Outline clip
  display.drawRect(x - 1, gapY - 4, pipeWidth + 2, 4, SH110X_WHITE);

  // --- BOTTOM PIPE ---
  int bottomHeight = groundY - bottomY;
  if (bottomHeight > 4) {
    display.drawRect(x, bottomY + 3, pipeWidth, bottomHeight - 3, SH110X_WHITE);
    display.drawFastVLine(x + 2, bottomY + 3, bottomHeight - 3, SH110X_WHITE); // 3D Highlight
  }
  // Bottom Pipe Lip (Rim)
  display.fillRect(x - 1, bottomY, pipeWidth + 2, 4, SH110X_WHITE);
}

// Draws moving grass/ground at the bottom
void drawGround(bool moving) {
  display.drawFastHLine(0, groundY, SCREEN_WIDTH, SH110X_WHITE);
  if (moving) {
    groundOffset = (groundOffset + 2) % 6;
  }
  for (int i = -groundOffset; i < SCREEN_WIDTH; i += 6) {
    display.drawLine(i, groundY + 1, i - 3, SCREEN_HEIGHT - 1, SH110X_WHITE);
  }
}

void triggerCrash() {
  // Brief screen flash for realistic hit impact
  display.invertDisplay(true);
  display.display();
  delay(50);
  display.invertDisplay(false);
  
  state = DYING;
  birdVelocity = 1.0; // Start falling down
}

void loop() {
  if (millis() - lastFrameTime < frameDelay) return;
  lastFrameTime = millis();

  bool buttonPressed = isButtonJustPressed();
  display.clearDisplay();

  // --- 1. START SCREEN ---
  if (state == START_SCREEN) {
    // Floating bird animation
    int floatY = 26 + (int)(sin(millis() / 180.0) * 3);
    drawBird(58, floatY);
    drawGround(true);

    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(28, 8);
    display.print("FLAPPY BIRD");

    // Blinking prompt
    if ((millis() / 400) % 2 == 0) {
      display.setCursor(22, 44);
      display.print("PRESS TO START");
    }

    if (buttonPressed) {
      resetGame();
      birdVelocity = jumpForce;
      state = PLAYING;
    }
  } 
  
  // --- 2. MAIN GAMEPLAY ---
  else if (state == PLAYING) {
    if (buttonPressed) {
      birdVelocity = jumpForce;
    }

    // Physics
    birdVelocity += gravity;
    if (birdVelocity > 3.5) birdVelocity = 3.5; // Terminal velocity
    birdY += birdVelocity;

    // Move Pipes
    pipeX -= pipeSpeed;
    if (pipeX < -pipeWidth) {
      pipeX = SCREEN_WIDTH;
      pipeGapY = random(8, groundY - pipeGapHeight - 8);
      passedPipe = false;
      if (pipeSpeed < 2.8) pipeSpeed += 0.04; // Gradually faster
    }

    // Score Check
    if (!passedPipe && (pipeX + pipeWidth) < birdX) {
      score++;
      passedPipe = true;
      if (score > highScore) highScore = score;
    }

    // Draw Scene
    drawPipes((int)pipeX, pipeGapY);
    drawGround(true);
    drawBird(birdX, (int)birdY);

    // Live Score Box (Top Left with black backing so it's always readable)
    display.fillRect(2, 2, 18, 11, SH110X_BLACK);
    display.setCursor(4, 4);
    display.setTextColor(SH110X_WHITE);
    display.print(score);

    // Collision Check: Ceiling & Ground
    if (birdY <= 0) birdY = 0;
    if (birdY + birdH >= groundY) {
      birdY = groundY - birdH;
      triggerCrash();
    }

    // Collision Check: Pipes (Slightly forgiving hitbox by 1px for better feel)
    if (birdX + birdW - 1 > (int)pipeX && birdX + 1 < (int)pipeX + pipeWidth) {
      if (birdY + 1 < pipeGapY || birdY + birdH - 1 > pipeGapY + pipeGapHeight) {
        triggerCrash();
      }
    }
  }

  // --- 3. DYING ANIMATION (Bird falls to the ground after hitting pipe) ---
  else if (state == DYING) {
    birdVelocity += 0.45; // Fast drop
    birdY += birdVelocity;

    if (birdY + birdH >= groundY) {
      birdY = groundY - birdH;
      state = GAME_OVER;
      stateTimer = millis(); // Start cooldown timer
    }

    drawPipes((int)pipeX, pipeGapY);
    drawGround(false);
    drawBird(birdX, (int)birdY);
  }

  // --- 4. GAME OVER POPUP (Compact & Perfectly Fitted) ---
  else if (state == GAME_OVER) {
    // Keep background visible
    drawPipes((int)pipeX, pipeGapY);
    drawGround(false);
    drawBird(birdX, (int)birdY);

    // Clear center area & draw double-border popup box (88x48 pixels, centered)
    display.fillRect(20, 6, 88, 48, SH110X_BLACK);
    display.drawRect(20, 6, 88, 48, SH110X_WHITE);
    display.drawRect(22, 8, 84, 44, SH110X_WHITE);

    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Title (9 chars = 54px wide -> centered at X=37)
    display.setCursor(37, 12);
    display.print("GAME OVER");
    display.drawFastHLine(28, 21, 72, SH110X_WHITE);

    // Scores (Well inside X=22 to X=106)
    display.setCursor(30, 25);
    display.print("Score : ");
    display.print(score);

    display.setCursor(30, 34);
    display.print("Best  : ");
    display.print(highScore);

    // Blinking Retry text after 500ms cooldown
    if (millis() - stateTimer > 500) {
      if ((millis() / 350) % 2 == 0) {
        display.setCursor(31, 43);
        display.print("PRESS BUTTON");
      }
      if (buttonPressed) {
        resetGame();
        birdVelocity = jumpForce;
        state = PLAYING;
      }
    }
  }

  display.display();
}