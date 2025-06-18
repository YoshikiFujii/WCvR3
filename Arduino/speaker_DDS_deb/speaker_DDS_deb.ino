#include <stdint.h>
#define F_CPU 16000000UL

//---------------設定項目---------------
const uint16_t SYNC_FREQ = 7000;  // 同期周波数（Hz）
const uint16_t dataFreqs[] = {3000, 3500, 4000, 4500}; // データ信号の周波数
const uint8_t targetCount = 4;
const uint16_t SampleRate = 16000; // 割り込みの分周比 2000000/(1+OCR2Anum*2)
//---------------内部係数---------------
//sinテーブル
const int8_t sineTable[32] = {
  128, 152, 176, 198, 218, 234, 245, 251,
  255, 251, 245, 234, 218, 198, 176, 152,
  128, 104, 80,  58,  38,  22,  11,   5,
    0,   5, 11,  22,  38,  58,  80, 104
};
const uint8_t PHASE_SCALE = 32;  // テーブルサイズ

volatile uint32_t syncPhaseStep = 0; //何回の割り込みでphaseを進めるか
volatile uint32_t dataPhaseStep = 0; 

volatile uint32_t syncPhase = 0; //同期信号の現在の位相位置
volatile uint32_t dataPhase = 0; //データ信号の位相位置

volatile uint16_t ISRcount = 0; //割り込み回数カウンタ

uint8_t currentTone = 0; //テスト用のプログラムの変数：一定間隔で変更する周波数の現在値
unsigned long lastSwitch = 0; //送信間隔
//-------------------------------------


//----------信号送信設定(レジスタへの書き込み)----------
void setup() {
  pinMode(9, OUTPUT);

  // PWM設定（Timer1）
  TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);

  // Timer2: 割り込み224kHz（7000Hz × 32）prescaler=8
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);  // prescaler=1 クロックそのままで16MHz
  OCR2A = (F_CPU/8/SampleRate) - 1;  // 16,000,000/(1 + OCR2A*2) 
  TIMSK2 = _BV(OCIE2A);

  // 初期位相ステップ設定
  syncPhaseStep = (uint16_t)(SampleRate / SYNC_FREQ);
  dataPhaseStep = (uint16_t)(SampleRate / dataFreqs[currentTone]);
  sei();  // グローバル割り込み有効化
}

//----------割り込みのたびに実行----------
ISR(TIMER2_COMPA_vect) {
  if(ISRcount % syncPhaseStep == 0){
    syncPhase += 1;
    if (syncPhase >= PHASE_SCALE)
        syncPhase -= PHASE_SCALE;
  }

  if(ISRcount % dataPhaseStep == 0){
    dataPhase += 1;
    if (dataPhase >= PHASE_SCALE)
        dataPhase -= PHASE_SCALE;
  }

  int8_t s1 = sineTable[syncPhase];
  int8_t s2 = sineTable[dataPhase];

  uint16_t sum = (uint16_t)s1 + (uint16_t)s2;
  OCR1A = sum >> 1;  // 255に収まるよう1ビットシフト
  ISRcount++;
}
//----------送信する周波数の遷移----------
void loop() {
  /*
  if (millis() - lastSwitch >= 200) {
  currentTone = (currentTone + 1) % targetCount;
  dataPhaseStep = SampleRate / dataFreqs[currentTone];
  lastSwitch = millis();
  */
  }

