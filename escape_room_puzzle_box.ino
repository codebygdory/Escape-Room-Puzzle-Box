#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo lockServo;

// ---------- Pins ----------
const int joyX = A1;
const int joyY = A2;
const int joySW = 3;
const int lightPin = A0;
const int buzzerPin = 8;
const int redPin = 11;
const int greenPin = 10;
const int bluePin = 9;
const int servoPin = 12;

// ---------- Puzzle stage tracking ----------
enum Stage { STAGE1_JOYSTICK, STAGE2_LIGHT, STAGE3_PASSCODE, UNLOCKED };
Stage currentStage = STAGE1_JOYSTICK;

// ---------- Stage 1: Joystick Direction Sequence ----------
const int stage1Length = 4;
int stage1Sequence[stage1Length];
int stage1PlayerIndex = 0;

// ---------- Stage 2: Light Pattern ----------
const int stage2Length = 3;
int stage2Sequence[stage2Length]; // 0 = cover (dark), 1 = uncover (bright)
int stage2PlayerIndex = 0;
unsigned long stage2StepStartTime = 0;
const unsigned long stage2TimeWindow = 4000; // 4 sec per step
const int lightDarkThreshold = 300;

// ---------- Stage 3: Passcode (joystick directions) ----------
const int stage3Length = 4;
int stage3Sequence[stage3Length];
int stage3PlayerIndex = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Serial is working!");
  lcd.init();
  lcd.backlight();
  pinMode(joySW, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  lockServo.attach(servoPin);
  lockServo.write(0); // locked position

  randomSeed(analogRead(A5)); // A5 is free-floating enough for a seed here

  lcd.setCursor(0, 0);
  lcd.print("Escape Box!");
  lcd.setCursor(0, 1);
  lcd.print("Solve to open");
  delay(2000);

  startStage1();
}

void loop() {
  switch (currentStage) {
    case STAGE1_JOYSTICK: handleStage1(); break;
    case STAGE2_LIGHT: handleStage2(); break;
    case STAGE3_PASSCODE: handleStage3(); break;
    case UNLOCKED: /* do nothing, box stays open */ break;
  }
  delay(50);
}

// ================= SHARED HELPERS =================

int readJoystickDirection() {
  int xVal = analogRead(joyX);
  int yVal = analogRead(joyY);

  if (yVal > 700) { delay(250); return 0; } // up
  if (yVal < 300) { delay(250); return 1; } // down
  if (xVal < 300) { delay(250); return 2; } // left
  if (xVal > 700) { delay(250); return 3; } // right
  return -1;
}

void setColor(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

void playWinJingle() {
  int notes[] = {523, 659, 784, 1047};
  int durations[] = {100, 100, 100, 250};
  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, notes[i], durations[i]);
    delay(durations[i] + 20);
  }
}

void playLoseJingle() {
  int notes[] = {392, 349, 294, 196};
  int durations[] = {150, 150, 150, 350};
  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, notes[i], durations[i]);
    delay(durations[i] + 20);
  }
}

void playBlipSound() {
  tone(buzzerPin, 1800, 40);
}

void directionFeedback(int dir) {
  if (dir == 0) { setColor(0, 0, 255); tone(buzzerPin, 400, 150); }      // up = blue
  else if (dir == 1) { setColor(255, 0, 0); tone(buzzerPin, 300, 150); } // down = red
  else if (dir == 2) { setColor(0, 255, 0); tone(buzzerPin, 500, 150); } // left = green
  else { setColor(255, 255, 0); tone(buzzerPin, 600, 150); }             // right = yellow
  delay(150);
  setColor(0, 0, 0);
  noTone(buzzerPin);
}

String directionName(int dir) {
  if (dir == 0) return "UP";
  if (dir == 1) return "DOWN";
  if (dir == 2) return "LEFT";
  return "RIGHT";
}

// ================= STAGE 1: JOYSTICK SEQUENCE =================

void startStage1() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stage 1: Moves");

  for (int i = 0; i < stage1Length; i++) {
    stage1Sequence[i] = random(0, 4);
  }
  stage1PlayerIndex = 0;

  lcd.setCursor(0, 1);
  lcd.print("Watch closely!");
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Memorize:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < stage1Length; i++) {
    directionFeedback(stage1Sequence[i]);
    lcd.print(directionName(stage1Sequence[i]).charAt(0));
    lcd.print(" ");
    delay(400);
  }
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Repeat it now!");
  lcd.setCursor(0, 1);
  lcd.print("Progress: 0/");
  lcd.print(stage1Length);
}

void handleStage1() {
  int input = readJoystickDirection();
  if (input == -1) return;

  directionFeedback(input);

  if (input == stage1Sequence[stage1PlayerIndex]) {
    stage1PlayerIndex++;
    lcd.setCursor(0, 1);
    lcd.print("Progress: ");
    lcd.print(stage1PlayerIndex);
    lcd.print("/");
    lcd.print(stage1Length);
    lcd.print("   ");

    if (stage1PlayerIndex >= stage1Length) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stage 1 Clear!");
      playWinJingle();
      delay(1500);
      startStage2();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong! Retry...");
    playLoseJingle();
    delay(1500);
    startStage1();
  }
}

// ================= STAGE 2: LIGHT PATTERN =================

void startStage2() {
  currentStage = STAGE2_LIGHT;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stage 2: Light");

  for (int i = 0; i < stage2Length; i++) {
    stage2Sequence[i] = random(0, 2);
  }
  stage2PlayerIndex = 0;

  lcd.setCursor(0, 1);
  lcd.print("Get ready...");
  delay(1500);

  showStage2Prompt();
}

void showStage2Prompt() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Step ");
  lcd.print(stage2PlayerIndex + 1);
  lcd.print("/");
  lcd.print(stage2Length);
  lcd.print(": ");
  lcd.print(stage2Sequence[stage2PlayerIndex] == 0 ? "COVER" : "UNCOVER");
  lcd.setCursor(0, 1);
  lcd.print("it now!");
  stage2StepStartTime = millis();
}

void handleStage2() {
  int lightLevel = analogRead(lightPin);
  Serial.print("Light: ");
  Serial.println(lightLevel);

  bool isDark = (lightLevel > lightDarkThreshold);
  int desired = stage2Sequence[stage2PlayerIndex];

  bool matched = (desired == 0 && isDark) || (desired == 1 && !isDark);

  if (matched) {
    playBlipSound();
    setColor(0, 255, 0);
    delay(150);
    setColor(0, 0, 0);

    stage2PlayerIndex++;
    if (stage2PlayerIndex >= stage2Length) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stage 2 Clear!");
      playWinJingle();
      delay(1500);
      startStage3();
    } else {
      delay(600);
      showStage2Prompt();
    }
    return;
  }

  if (millis() - stage2StepStartTime > stage2TimeWindow) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Too slow! Retry");
    playLoseJingle();
    delay(1500);
    startStage2();
  }
}

// ================= STAGE 3: PASSCODE + SERVO UNLOCK =================

void startStage3() {
  currentStage = STAGE3_PASSCODE;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stage 3: Code");

  for (int i = 0; i < stage3Length; i++) {
    stage3Sequence[i] = random(0, 4);
  }
  stage3PlayerIndex = 0;

  lcd.setCursor(0, 1);
  lcd.print("Watch closely!");
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Memorize:");
  lcd.setCursor(0, 1);
  for (int i = 0; i < stage3Length; i++) {
    directionFeedback(stage3Sequence[i]);
    lcd.print(directionName(stage3Sequence[i]).charAt(0));
    lcd.print(" ");
    delay(400);
  }
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter passcode!");
  lcd.setCursor(0, 1);
  lcd.print("Progress: 0/");
  lcd.print(stage3Length);
}

void handleStage3() {
  int input = readJoystickDirection();
  if (input == -1) return;

  directionFeedback(input);

  if (input == stage3Sequence[stage3PlayerIndex]) {
    stage3PlayerIndex++;
    lcd.setCursor(0, 1);
    lcd.print("Progress: ");
    lcd.print(stage3PlayerIndex);
    lcd.print("/");
    lcd.print(stage3Length);
    lcd.print("   ");

    if (stage3PlayerIndex >= stage3Length) {
      unlockBox();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong! Retry...");
    playLoseJingle();
    delay(1500);
    startStage3();
  }
}

void unlockBox() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UNLOCKED!");
  lcd.setCursor(0, 1);
  lcd.print("Box is open!");
  
  lockServo.write(90); // Open the servo latch (adjust angle if needed)
  currentStage = UNLOCKED;
  
  // Big winning celebration
  for(int i = 0; i < 3; i++) {
    setColor(0, 255, 0);
    playWinJingle();
    setColor(0, 0, 0);
    delay(200);
  }

  // Stay open for a while before relocking
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Relocking in...");
  for (int i = 10; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print(i);
    lcd.print(" sec    ");
    delay(1000);
  }

  // Relock and reset the puzzle
  lockServo.write(0); // back to locked position
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Relocked!");
  playLoseJingle(); // gives a little "closing" sound cue
  delay(1500);

  startStage1();
}