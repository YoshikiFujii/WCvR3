const uint8_t sineTable[64] = {
  128,140,152,165,176,187,198,208,
  218,226,234,241,247,251,254,255,
  255,254,251,247,241,234,226,218,
  208,198,187,176,165,152,140,128,
  116,104, 91, 80, 69, 58, 48, 38,
   30, 22, 15,  9,  5,  2,  1,  0,
    0,  1,  2,  5,  9, 15, 22, 30,
   38, 48, 58, 69, 80, 91,104,116
};

struct ToneSetting {
  uint16_t freq;
  float updateIntervalMicros;  // マイクロ秒単位
};

ToneSetting tones[] = {
  {3000, 0}, {3500, 0}, {4000, 0}, {4500, 0}
};

uint8_t currentTone = 0;
uint8_t idx = 0;
unsigned long lastUpdateMicros = 0;
unsigned long lastSwitchMicros = 0;

void setup() {
  pinMode(9, OUTPUT);
  // Fast PWM設定（Timer1）
  TCCR1A = _BV(COM1A1) | _BV(WGM10); // 8bit Fast PWM
  TCCR1B = _BV(WGM12) | _BV(CS10);   // クロック分周なし（16MHz）

  // 各トーンに対応するテーブル更新周期を事前計算
  for (int i = 0; i < 4; i++) {
    tones[i].updateIntervalMicros = 1e6 / (tones[i].freq * 64.0);
  }
}

void loop() {
  unsigned long nowMicros = micros();

  // サイン波出力制御
  if (nowMicros - lastUpdateMicros >= tones[currentTone].updateIntervalMicros) {
    OCR1A = sineTable[idx];
    idx = (idx + 1) & 0x3F; // 64点ループ（高速化）
    lastUpdateMicros = nowMicros;
  }

  // 音の切り替え（200msごと）
  if (nowMicros - lastSwitchMicros >= 200000) {
    currentTone = (currentTone + 1) % 4;
    lastSwitchMicros = nowMicros;
  }
}
