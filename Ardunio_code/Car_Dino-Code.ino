#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ==========================================
// System & Display Configuration
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Network configuration
const char* ssid = "CAR_GAME"; // NodeMCU acts as an Access Point
const char* password = "12345678";

ESP8266WebServer server(80);

// Global Game State Variables
int gameState = 0;       // 0 = Selección, 1 = Car Game, 2 = Dino Game
int score = 0;
bool gameOver = false;
bool gameRunning = false; // NEW: true only after Start/Restart is pressed
int carHighScore = 0;
int dinoHighScore = 0;

// Shared Bitmaps (8x8 pixels)
const unsigned char carBitmap[] PROGMEM = {
    0b00011000, 0b00111100, 0b01111110, 0b11011011,
    0b11111111, 0b01111110, 0b00100100, 0b01000010
};

const unsigned char dinoBitmap[] PROGMEM = {
    0b00001110, 0b00001101, 0b00001111, 0b01111100,
    0b01111111, 0b00111100, 0b00010100, 0b00010100
};

const unsigned char cactusBitmap[] PROGMEM = {
    0b00100100, 0b01101100, 0b01111100, 0b01101100,
    0b01101100, 0b01101100, 0b00111000, 0b00111000
};

// ==========================================
// CAR GAME Logic & Endpoints (image_9.png logic)
// ==========================================
const int LEFT_LANE = 38;
const int RIGHT_LANE = 82;
int roadOffset = 0;
int playerLane = 0;
int playerX = LEFT_LANE;
const int playerY = 52;
int enemyLane, enemyX, enemyY;

void resetCarGame() {
    score = 0;
    gameOver = false;
    roadOffset = 0;
    playerLane = 0;
    playerX = LEFT_LANE;
    enemyLane = random(0, 2);
    enemyX = (enemyLane == 0) ? LEFT_LANE : RIGHT_LANE;
    enemyY = -8;
}

void showCarGameOver() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(30, 5); display.print("CRASH!");

    display.setTextSize(1);
    display.setCursor(35, 30); display.print("Score: "); display.print(score);
    display.setCursor(35, 42); display.print("HiScore: "); display.print(carHighScore);
    display.setCursor(25, 55); display.print("Press RESTART");
    display.display();
}

// NEW: shown while waiting for the Start button
void showCarWaitScreen() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 15); display.print("CAR GAME");

    display.setTextSize(1);
    display.setCursor(15, 40); display.print("Press START to play");
    display.setCursor(10, 52); display.print("HiScore: "); display.print(carHighScore);
    display.display();
}

void playCarGame() {
    // Road animation
    roadOffset++;
    if (roadOffset >= 16) roadOffset = 0;

    // Enemy movement
    enemyY += 2;
    if (enemyY > SCREEN_HEIGHT) {
        score++;
        enemyLane = random(0, 2);
        enemyX = (enemyLane == 0) ? LEFT_LANE : RIGHT_LANE;
        enemyY = -8;
    }

    // Collision Check (lane based for Car game)
    if (enemyLane == playerLane && (enemyY + 8 >= playerY) && (enemyY <= playerY + 8)) {
        gameOver = true;
        gameRunning = false; // stop the loop from playing; wait for next Start command
        if (score > carHighScore) carHighScore = score;
        showCarGameOver();
        return;
    }

    display.clearDisplay();
    // Draw Road (boundaries and dotted lines)
    display.drawLine(20, 0, 20, 63, WHITE);
    display.drawLine(108, 0, 108, 63, WHITE);
    for (int y = -8 + roadOffset; y < 64; y += 16) display.fillRect(63, y, 2, 8, WHITE);

    // Draw Cars
    display.drawBitmap(playerX, playerY, carBitmap, 8, 8, WHITE);
    display.drawBitmap(enemyX, enemyY, carBitmap, 8, 8, WHITE);

    // In-game score display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(2, 2); display.print("Score:"); display.print(score);
    display.display();
}

// ==========================================
// DINO GAME Logic & Endpoints (image_10.png logic)
// ==========================================
const int GROUND_Y = 56;
int dinoY = GROUND_Y - 8;
int dinoVelocityY = 0;
bool isJumping = false;
int cactusX = 128;
int cactusY = GROUND_Y - 8;

void resetDinoGame() {
    score = 0;
    gameOver = false;
    dinoY = GROUND_Y - 8;
    dinoVelocityY = 0;
    isJumping = false;
    cactusX = 128;
}

void showDinoGameOver() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(25, 5); display.print("GOT YA!");

    display.setTextSize(1);
    display.setCursor(35, 30); display.print("Score: "); display.print(score);
    display.setCursor(35, 42); display.print("HiScore: "); display.print(dinoHighScore);
    display.setCursor(25, 55); display.print("Press RESTART");
    display.display();
}

// NEW: shown while waiting for the Start button
void showDinoWaitScreen() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(15, 15); display.print("DINO GAME");

    display.setTextSize(1);
    display.setCursor(15, 40); display.print("Press START to play");
    display.setCursor(10, 52); display.print("HiScore: "); display.print(dinoHighScore);
    display.display();
}

void playDinoGame() {
    // Gravity & Jump Physics
    if (isJumping) {
        dinoY += dinoVelocityY;
        dinoVelocityY += 1; // Gravity simulation
        if (dinoY >= GROUND_Y - 8) {
            dinoY = GROUND_Y - 8;
            isJumping = false;
        }
    }

    // Move Cactus
    cactusX -= 3;
    if (cactusX < -8) {
        cactusX = 128;
        score++;
    }

    // Collision Check (Requirement: detect collision with fixed player at X=16)
    // The cactus must be within fixed dino boundaries (X=16, height=8)
    if ((cactusX <= 16 + 6 && cactusX + 6 >= 16) && (dinoY + 8 >= cactusY)) {
        gameOver = true;
        gameRunning = false; // stop the loop from playing; wait for next Start command
        if (score > dinoHighScore) dinoHighScore = score;
        showDinoGameOver();
        return;
    }

    display.clearDisplay();
    // Draw Environment
    display.drawLine(0, GROUND_Y, 128, GROUND_Y, WHITE); // Ground line

    // Draw Sprites
    display.drawBitmap(16, dinoY, dinoBitmap, 8, 8, WHITE); // Fixed Dino position at X=16
    display.drawBitmap(cactusX, cactusY, cactusBitmap, 8, 8, WHITE);

    // In-game score display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(2, 2); display.print("Score:"); display.print(score);
    display.display();
}

// ==========================================
// UNIVERSAL SYSTEM MENUS & WIFI Setup
// ==========================================
void showSelectionMenu() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 5); display.print("Games 💕");

    display.setTextSize(1);
    display.setCursor(5, 30); display.print("Car High Score: "); display.print(carHighScore);
    display.setCursor(5, 42); display.print("Dino High Score: "); display.print(dinoHighScore);
    display.setCursor(10, 55); display.print("Select Game on App");
    display.display();
}

// ==========================================
// Server Callbacks matching your images
// ==========================================

// Commands to open screens (called when leaving the selection menu, per Screen1)
// NOTE: these only OPEN the screen now. They reset the game state but do NOT
// start it running — the player must press Start/Restart to actually play.
void handlePrepareCarScreen() {
    Serial.println("CMD: OPEN CAR SCREEN");
    gameState = 1;
    resetCarGame();
    gameRunning = false;
    server.send(200, "text/plain", "SCREEN_CAR_OPEN");
}

void handlePrepareDinoScreen() {
    Serial.println("CMD: OPEN DINO SCREEN");
    gameState = 2;
    resetDinoGame();
    gameRunning = false;
    server.send(200, "text/plain", "SCREEN_DINO_OPEN");
}

// Commands called by START/RESTART buttons (called from within game screens)
void handlePlayCar() {
    Serial.println("CMD: CAR START/RESTART");
    resetCarGame();
    gameRunning = true;
    server.send(200, "text/plain", "CAR_RESTARTED");
}

void handlePlayDino() {
    Serial.println("CMD: DINO START/RESTART");
    resetDinoGame();
    gameRunning = true;
    server.send(200, "text/plain", "DINO_RESTARTED");
}

// In-game controls
void handleMoveLeft() {
    Serial.println("CMD: LEFT");
    if (gameState == 1) { playerLane = 0; playerX = LEFT_LANE; }
    server.send(200, "text/plain", "LEFT");
}

void handleMoveRight() {
    Serial.println("CMD: RIGHT");
    if (gameState == 1) { playerLane = 1; playerX = RIGHT_LANE; }
    server.send(200, "text/plain", "RIGHT");
}

void handleJump() {
    Serial.println("CMD: JUMP");
    if (gameState == 2 && !isJumping) { isJumping = true; dinoVelocityY = -7; }
    server.send(200, "text/plain", "JUMP");
}

// Return to Main Menu (called by MAIN MENU buttons)
void handleReturnToMenu() {
    Serial.println("CMD: RETURN TO MENU");
    gameState = 0;
    gameRunning = false;
    server.send(200, "text/plain", "MENU");
}

void setup() {
    Serial.begin(115200);
    Wire.begin(D2, D1); // Explicitly stating I2C pins for clarity

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED initialization failed!");
        while (true);
    }

    display.clearDisplay();
    display.display();

    // Use analog noise for a truly random enemy/cactus sequence
    randomSeed(analogRead(A0));

    // WiFi AP setup
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    Serial.println("WiFi AP Started");
    Serial.print("Connect to network: "); Serial.println(ssid);

    // Endpoint Configuration matching App Inventor Blocks (images_8-10.png)
    server.on("/screen_car", handlePrepareCarScreen);   // Open Screen2 (waits for Start)
    server.on("/screen_dino", handlePrepareDinoScreen); // Open Screen3 (waits for Start)
    server.on("/menu", handleReturnToMenu);            // Back to selection (image_9/10)

    server.on("/play_car", handlePlayCar);              // Start/Restart Car (image_9)
    server.on("/play_dino", handlePlayDino);            // Start/Restart Dino (image_10)

    server.on("/left", handleMoveLeft);                 // Car controls (image_9)
    server.on("/right", handleMoveRight);
    server.on("/jump", handleJump);                     // Dino controls (image_10)

    server.begin();
}

void loop() {
    server.handleClient(); // Handle App commands

    // Check if the game is over. If so, wait for Restart in App.
    if (gameOver) {
        delay(100);
        return;
    }

    // Main system state machine
    if (gameState == 0) {
        showSelectionMenu();
        delay(100); // Slow refresh rate for selection menu
    } else if (gameState == 1) {
        if (gameRunning) {
            playCarGame();  // Car logic
            delay(30);      // Car game speed
        } else {
            showCarWaitScreen(); // Waiting for Start button
            delay(100);
        }
    } else if (gameState == 2) {
        if (gameRunning) {
            playDinoGame(); // Dino logic
            delay(30);       // Dino game speed
        } else {
            showDinoWaitScreen(); // Waiting for Start button
            delay(100);
        }
    }
}
