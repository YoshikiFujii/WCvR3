//---------------設定項目---------------
const float BASE_FREQ = 7000.0;  // 同期周波数（Hz）
const float dataFreqs[] = {3000.0, 3500.0, 4000.0, 4500.0}; // データ信号の周波数
const uint8_t targetCount = 4;
//---------------内部係数---------------
//sinテーブル
const uint8_t sineTable[32] = {
  128, 152, 176, 198, 218, 234, 245, 251,
  255, 251, 245, 234, 218, 198, 176, 152,
  128, 104, 80,  58,  38,  22,  11,   5,
    0,   5, 11,  22,  38,  58,  80, 104
};
const float PHASE_SCALE = 32.0;  // テーブルサイズ
const float STEP_FREQ = BASE_FREQ * PHASE_SCALE;  // 割り込み周波数（Hz）→ 224000Hz

float phaseStep = 0.0; //毎回の割り込みでどれだけphaseを進めるか  !!!!setup()ですべての周波数を計算しておくのも試す!!!!

volatile float syncPhase = 0.0; //同期信号の現在の位相位置
volatile float dataPhase = 0.0; //データ信号の位相位置

uint8_t currentTone = 0; //テスト用のプログラムの変数：一定間隔で変更する周波数の現在値
unsigned long lastSwitch = 0; //送信間隔
//-------------------------------------


//----------信号送信設定(レジスタへの書き込み)----------
void setup() {
  pinMode(9, OUTPUT);

  // PWM設定（Timer1）
  TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = sineTable[0];

  // Timer2: 割り込み224kHz（7000Hz × 32）
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);  // 分周8 → 2MHz
  OCR2A = 8;  // 2MHz / (8+1) = 222222Hz ≒ 224kHz
  TIMSK2 = _BV(OCIE2A);

  // 初期位相ステップ設定
  //phaseStep = dataFreqs[currentTone] / BASE_FREQ;
  phaseStep = dataFreqs[currentTone] / STEP_FREQ * PHASE_SCALE;
}

//----------割り込みのたびに実行----------
ISR(TIMER2_COMPA_vect) {
  syncPhase += 1.0;
  if (syncPhase >= PHASE_SCALE) syncPhase -= PHASE_SCALE;

  dataPhase += phaseStep;
  if (dataPhase >= PHASE_SCALE) dataPhase -= PHASE_SCALE;

  uint8_t s1 = sineTable[(uint8_t)syncPhase];
  uint8_t s2 = sineTable[(uint8_t)dataPhase];

  OCR1A = (s1 + s2) / 2;
}

//----------送信する周波数の遷移----------
void loop() {
  if (millis() - lastSwitch >= 200) {
    currentTone = (currentTone + 1) % targetCount;
    //phaseStep = dataFreqs[currentTone] / BASE_FREQ;
    phaseStep = dataFreqs[currentTone] / STEP_FREQ * PHASE_SCALE;
    lastSwitch = millis();
  }
}
