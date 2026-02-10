/*
 * 1D Game Box V10 - By Eddie 
 
 * 雙方各自同時按下兩顆按鈕啟動乒乓球
 * 雙方各自按下，下方按鈕五下 啟動城池戰
 */

#include <FastLED.h>

// ===== 硬體定義 =====
#define LED_PIN     6
#define NUM_LEDS    240
#define SPEAKER_PIN 10
const int P1_BTN_A = 2; const int P1_BTN_B = 3;
const int P2_BTN_A = 4; const int P2_BTN_B = 5;

CRGB leds[NUM_LEDS];

// ===== 系統狀態與計時器 =====
enum GameState { STATE_IDLE, STATE_PLAYING_PONG, STATE_CASTLE_WAR, STATE_GAME_OVER };
GameState currentState = STATE_IDLE;

unsigned long lastUpdateTick = 0;   
unsigned long lastAnimationTick = 0;
unsigned long lastSoundTick = 0;    
unsigned long gameTimer = 0;        

// ===== 按鈕狀態 =====
bool p1A, p1B, p2A, p2B;
bool p1A_old, p1B_old, p2A_old, p2B_old;
int p1B_Count = 0, p2B_Count = 0;
bool p1Ready = false, p2Ready = false;

// ===== 遊戲邏輯變數 =====
float ballPosition = 120.0;
float ballVelocity = 0.8;
float cargoPosition = 120.0;
int winnerID = 0;
int gameOverStep = 0;

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(80); 
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 3000); 
  
  pinMode(P1_BTN_A, INPUT_PULLUP); pinMode(P1_BTN_B, INPUT_PULLUP);
  pinMode(P2_BTN_A, INPUT_PULLUP); pinMode(P2_BTN_B, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  FastLED.clear(true);
}

void loop() {
  updateButtons(); 
  updateSound();   

  switch (currentState) {
    case STATE_IDLE:         runIdle(); break;
    case STATE_PLAYING_PONG: runPong(); break;
    case STATE_CASTLE_WAR:   runWar();  break;
    case STATE_GAME_OVER:    runGameOverAnim(); break;
  }
}

// ==========================================
// 輔助功能
// ==========================================
void updateButtons() {
  p1A_old = p1A; p1B_old = p1B; p2A_old = p2A; p2B_old = p2B;
  p1A = (digitalRead(P1_BTN_A) == LOW);
  p1B = (digitalRead(P1_BTN_B) == LOW);
  p2A = (digitalRead(P2_BTN_A) == LOW);
  p2B = (digitalRead(P2_BTN_B) == LOW);
}

bool p1A_clicked() { return p1A && !p1A_old; }
bool p1B_clicked() { return p1B && !p1B_old; }
bool p2A_clicked() { return p2A && !p2A_old; }
bool p2B_clicked() { return p2B && !p2B_old; }

void startSound(int freq, int duration) {
  tone(SPEAKER_PIN, freq);
  lastSoundTick = millis() + duration;
}

void updateSound() {
  if (millis() > lastSoundTick) noTone(SPEAKER_PIN);
}

// ==========================================
// 待機模式
// ==========================================
void runIdle() {
  if (p1B_clicked()) { p1B_Count++; startSound(440, 50); }
  if (p2B_clicked()) { p2B_Count++; startSound(440, 50); }

  if (p1B_Count >= 5 && p2B_Count >= 5) {
    p1B_Count = 0; p2B_Count = 0;
    startCastleWar(); return;
  }

  if (p1A && p1B) { p1Ready = true; startSound(880, 100); }
  if (p2A && p2B) { p2Ready = true; startSound(880, 100); }
  if (p1Ready && p2Ready) { startPong(); return; }

  if (millis() - lastAnimationTick > 50) {
    lastAnimationTick = millis();
    fadeToBlackBy(leds, NUM_LEDS, 20);
    leds[random(NUM_LEDS)] = CHSV(random8(), 255, 200);
    if(p1B_Count > 0) fill_solid(leds, p1B_Count*3, CRGB::Red);
    if(p2B_Count > 0) fill_solid(leds + (NUM_LEDS - p2B_Count*3), p2B_Count*3, CRGB::Red);
    if(p1Ready) fill_solid(leds, 10, CRGB::Blue);
    if(p2Ready) fill_solid(leds + 230, 10, CRGB::Blue);
    FastLED.show();
  }
}

// ==========================================
// Pong 模式
// ==========================================
void startPong() {
  currentState = STATE_PLAYING_PONG;
  ballPosition = 120.0;
  ballVelocity = (random(0,2)==0) ? 0.8 : -0.8;
  startSound(1046, 200);
}

void runPong() {
  bool p1Click = p1A_clicked() || p1B_clicked();
  bool p2Click = p2A_clicked() || p2B_clicked();
  int bp = round(ballPosition);

  // P1 擊球判定 (左側)
  if (ballVelocity < 0 && p1Click) {
    if (bp >= 4 && bp <= 9) { 
      ballVelocity = abs(ballVelocity) * 1.3; startSound(1500, 50); // 黃色加速
    } 
    else if (bp >= 10 && bp <= 18) { 
      ballVelocity = abs(ballVelocity) * 0.8; startSound(700, 30);  // 綠色減速
    }
  }

  // P2 擊球判定 (右側)
  if (ballVelocity > 0 && p2Click) {
    if (bp >= 230 && bp <= 235) { 
      ballVelocity = -abs(ballVelocity) * 1.3; startSound(1500, 50); // 黃色加速
    } 
    else if (bp >= 221 && bp <= 229) { // 綠
      ballVelocity = -abs(ballVelocity) * 0.8; startSound(700, 30);  // 綠色減速
    }
  }

  if (millis() - lastUpdateTick > 10) {
    lastUpdateTick = millis();
    ballPosition += ballVelocity;
    
    if (ballPosition < 0) { triggerGameOver(2); return; }
    if (ballPosition >= NUM_LEDS) { triggerGameOver(1); return; }

    FastLED.clear();
    drawZones();
    int renderBall = (int)round(ballPosition);
    if(renderBall >=0 && renderBall < NUM_LEDS) leds[renderBall] = CRGB::White;
    FastLED.show();
  }
}

void drawZones() {
  // P1 側 (左)
  for(int i=0; i<4; i++)  leds[i] = CRGB::Red;      
  for(int i=4; i<10; i++) leds[i] = CRGB::Yellow;   
  for(int i=10; i<19; i++) leds[i] = CRGB::Green;   // 10-18 (9顆)

  // P2 側 (右)
  for(int i=236; i<240; i++) leds[i] = CRGB::Red;    
  for(int i=230; i<236; i++) leds[i] = CRGB::Yellow; 
  for(int i=221; i<230; i++) leds[i] = CRGB::Green;  // 221-229 (9顆)
}

// ==========================================
// 城池戰
// ==========================================
void startCastleWar() {
  currentState = STATE_CASTLE_WAR;
  gameTimer = millis();
  cargoPosition = 120.0;
  startSound(1200, 400);
}

void runWar() {
  unsigned long elapsed = millis() - gameTimer;
  if (elapsed >= 30000) {
    if (cargoPosition < 120) triggerGameOver(2); 
    else if (cargoPosition > 120) triggerGameOver(1);
    else triggerGameOver(0);
    return;
  }
  if ( (p1A && p1B) && !(p1A_old && p1B_old) ) { cargoPosition += 2.0; startSound(700, 20); }
  if ( (p2A && p2B) && !(p2A_old && p2B_old) ) { cargoPosition -= 2.0; startSound(700, 20); }
  if (cargoPosition < 2) { triggerGameOver(2); return; }
  if (cargoPosition > 237) { triggerGameOver(1); return; }

  if (millis() - lastUpdateTick > 20) {
    lastUpdateTick = millis();
    FastLED.clear();
    int timeLeds = map(elapsed, 0, 30000, 0, NUM_LEDS);
    for(int i=0; i<timeLeds; i++) leds[i] = CRGB(40, 40, 40);
    int cp = round(cargoPosition);
    for(int i=cp-2; i<=cp+1; i++) if(i>=0 && i<NUM_LEDS) leds[i] = CRGB::Red;
    FastLED.show();
  }
}

void triggerGameOver(int winner) {
  winnerID = winner;
  currentState = STATE_GAME_OVER;
  gameOverStep = 0;
  lastUpdateTick = millis();
  startSound(winner == 0 ? 200 : 600, 500);
}

void runGameOverAnim() {
  if (millis() - lastUpdateTick > 400) {
    lastUpdateTick = millis();
    gameOverStep++;
    if (gameOverStep > 6) {
      p1Ready = p2Ready = false; p1B_Count = 0; p2B_Count = 0;
      currentState = STATE_IDLE;
      return;
    }
    if (gameOverStep % 2 == 1) {
      if (winnerID == 1) { fill_solid(leds, 120, CRGB::Green); fill_solid(leds+120, 120, CRGB::Red); }
      else if (winnerID == 2) { fill_solid(leds, 120, CRGB::Red); fill_solid(leds+120, 120, CRGB::Green); }
      else fill_solid(leds, NUM_LEDS, CRGB::Yellow);
    } else { FastLED.clear(); }
    FastLED.show();
  }
}