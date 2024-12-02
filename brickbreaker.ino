#include <Adafruit_Arcada.h>
#include <Adafruit_LSM6DS33.h>

Adafruit_Arcada arcada;
Adafruit_LSM6DS33 lsm6ds33;

// Paddle properties
#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 8
#define PADDLE_Y (240 - 20)
#define MOVE_SPEED 7.5  // Increased from 5 to 7.5 (1.5x faster)
int paddleX;

// Ball properties
#define BALL_SIZE 6
float ballX, ballY;    
float ballSpeedX = 6;  
float ballSpeedY = 6;  

// Brick properties
#define BRICK_WIDTH 30
#define BRICK_HEIGHT 10
#define BRICK_GAP 5
#define NUM_BRICKS 7
#define NUM_ROWS 3
#define BRICK_START_Y 20
bool bricks[NUM_ROWS][NUM_BRICKS];


// Game state
bool gameOver = false;
bool gameWon = false;
unsigned long gameOverStartTime = 0;
#define RESTART_DELAY 5000  // Reduced from 10000 to 5000 (5 seconds)

// Accelerometer threshold
#define TILT_THRESHOLD 2.0

bool checkWin() {
  for(int row = 0; row < NUM_ROWS; row++) {
    for(int col = 0; col < NUM_BRICKS; col++) {
      if(bricks[row][col]) return false;  // Found a brick still visible
    }
  }
  return true;  // All bricks are gone
}

void resetGame() {
  paddleX = (240 - PADDLE_WIDTH) / 2;
  ballX = 240/2;
  ballY = 240/2;
  ballSpeedX = 6;
  ballSpeedY = 6;
  gameOver = false;
  gameWon = false;

  // Reset all bricks to visible
  for(int row = 0; row < NUM_ROWS; row++) {
    for(int col = 0; col < NUM_BRICKS; col++) {
      bricks[row][col] = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  if (!arcada.arcadaBegin()) {
    Serial.println("Failed to begin");
    while (1);
  }
  
  if (!lsm6ds33.begin_I2C()) {
    Serial.println("Failed to find LSM6DS33 chip");
    while (1);
  }
  
  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);
  arcada.display->setRotation(1);
  
  resetGame();
  
  Serial.println("Setup complete");
}

void checkBrickCollision() {
  for(int row = 0; row < NUM_ROWS; row++) {
    int brickY = BRICK_START_Y + (row * (BRICK_HEIGHT + BRICK_GAP));
    if (ballY >= brickY && ballY <= brickY + BRICK_HEIGHT) {
      for(int col = 0; col < NUM_BRICKS; col++) {
        if (bricks[row][col]) {
          int brickX = col * (BRICK_WIDTH + BRICK_GAP) + BRICK_GAP;
          if (ballX >= brickX && ballX <= brickX + BRICK_WIDTH) {
            bricks[row][col] = false;
            ballSpeedY = -ballSpeedY;
            
            // Check for win after breaking a brick
            if (checkWin()) {
              gameWon = true;
              gameOver = true;
              gameOverStartTime = millis();
            }
            return;
          }
        }
      }
    }
  }
}

void drawBricks() {
  for(int row = 0; row < NUM_ROWS; row++) {
    int brickY = BRICK_START_Y + (row * (BRICK_HEIGHT + BRICK_GAP));
    for(int col = 0; col < NUM_BRICKS; col++) {
      if(bricks[row][col]) {
        int brickX = col * (BRICK_WIDTH + BRICK_GAP) + BRICK_GAP;
        arcada.display->fillRect(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT, ARCADA_WHITE);
      }
    }
  }
}

void checkPaddleCollision() {
  if (ballY >= PADDLE_Y - BALL_SIZE && ballY <= PADDLE_Y + PADDLE_HEIGHT) {
    if (ballX >= paddleX && ballX <= paddleX + PADDLE_WIDTH) {
      float hitPosition = (ballX - paddleX) / PADDLE_WIDTH;
      ballSpeedY = -abs(ballSpeedY);
      float newSpeedX = (hitPosition - 0.5) * 12;
      ballSpeedX = newSpeedX;
      ballY = PADDLE_Y - BALL_SIZE;
    }
  }
}

void updateBall() {
  if (gameOver) return;
  
  ballX += ballSpeedX;
  ballY += ballSpeedY;
  
  if (ballX <= 0) {
    ballX = 0;
    ballSpeedX = abs(ballSpeedX);
  }
  if (ballX >= (240 - BALL_SIZE)) {
    ballX = 240 - BALL_SIZE;
    ballSpeedX = -abs(ballSpeedX);
  }
  if (ballY <= 0) {
    ballY = 0;
    ballSpeedY = abs(ballSpeedY);
  }
  
  if (ballY >= (240 - BALL_SIZE)) {
    gameOver = true;
    gameOverStartTime = millis();
    return;
  }
  
  checkBrickCollision();
  checkPaddleCollision();
}

void loop() {
  if (gameOver && (millis() - gameOverStartTime >= RESTART_DELAY)) {
    resetGame();
  }
  
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds33.getEvent(&accel, &gyro, &temp);
  
  if (accel.acceleration.x > TILT_THRESHOLD) {
    paddleX = min(paddleX + MOVE_SPEED, 240 - PADDLE_WIDTH);
  } 
  else if (accel.acceleration.x < -TILT_THRESHOLD) {
    paddleX = max(paddleX - MOVE_SPEED, 0);
  }
  
  updateBall();
  
  arcada.display->fillScreen(ARCADA_BLACK);
  
  drawBricks();
  
  arcada.display->fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, ARCADA_WHITE);
  
  arcada.display->fillCircle(ballX, ballY, BALL_SIZE/2, ARCADA_WHITE);
  
  if (gameOver) {
    arcada.display->setTextSize(3);
    if (gameWon) {
      arcada.display->setTextColor(ARCADA_GREEN);
      arcada.display->setCursor(45, 100);
      arcada.display->println("YOU WIN!");
    } else {
      arcada.display->setTextColor(ARCADA_RED);
      arcada.display->setCursor(45, 100);
      arcada.display->println("GAME OVER");
    }
  }
  
  delay(16);
}