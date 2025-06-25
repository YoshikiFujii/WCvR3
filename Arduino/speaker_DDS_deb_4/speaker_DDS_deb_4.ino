#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU        16000000UL
#define SAMPLE_RATE  16000UL   // サンプル周波数 16 kHz
#define CS_DDR   DDRB
#define CS_PORT  PORTB
#define CS_BIT   PB2   // Arduino D10

//const uint8_t PROGMEM SINE_TABLE[256] = { //SRAMではなく、フラッシュにリストを保存する場合
const uint8_t SINE_TABLE[256] = {
    128, 131, 134, 137, 140, 144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174,
    177, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 239, 240, 241, 243, 244,
    245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
    255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
    245, 244, 243, 241, 240, 239, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
    218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179,
    177, 174, 171, 168, 165, 162, 159, 156, 153, 150, 147, 144, 140, 137, 134, 131,
    128, 125, 122, 119, 116, 112, 109, 106, 103, 100,  97,  94,  91,  88,  85,  82,
     79,  77,  74,  71,  68,  65,  63,  60,  57,  55,  52,  50,  47,  45,  43,  40,
     38,  36,  34,  32,  30,  28,  26,  24,  22,  21,  19,  17,  16,  15,  13,  12,
     11,  10,   8,   7,   6,   6,   5,   4,   3,   3,   2,   2,   2,   1,   1,   1,
      1,   1,   1,   1,   2,   2,   2,   3,   3,   4,   5,   6,   6,   7,   8,  10,
     11,  12,  13,  15,  16,  17,  19,  21,  22,  24,  26,  28,  30,  32,  34,  36,
     38,  40,  43,  45,  47,  50,  52,  55,  57,  60,  63,  65,  68,  71,  74,  77,
     79,  82,  85,  88,  91,  94,  97, 100, 103, 106, 109, 112, 116, 119, 122, 125
};

volatile uint32_t phase1 = 0, phase2 = 0;  // 位相位置（番目）
uint32_t inc1, inc2;                      // 一回の位相移動個数

void spi_init() {
  // SPI端子設定：PB3(MOSI), PB5(SCK), PB2(CS)を出力に
  DDRB |= _BV(PB3) | _BV(PB5) | _BV(CS_BIT);

  // SPI初期化: マスター, MSB先, モード0, Fosc/4
  SPCR = _BV(SPE) | _BV(MSTR);   // SPI有効化、マスターモード
  SPSR = _BV(SPI2X);             // 倍速モードでF_CPU/2
}
void spi_send8(uint8_t data) {
  SPDR = data;
  while (!(SPSR & _BV(SPIF)));  // 送信完了待ち
}
void sendToDAC(uint16_t value) {
  uint16_t command = 0b0011000000000000 | (value & 0x0FFF);  // MCP4921 command
  CS_PORT &= ~_BV(CS_BIT);           // CSをLOWに
  spi_send8(command >> 8);           // 上位8ビット
  spi_send8(command & 0xFF);         // 下位8ビット
  CS_PORT |= _BV(CS_BIT);            // CSをHIGHに
}

ISR(TIMER2_COMPA_vect) {
  // フェーズを進める
  phase1 += inc1;
  phase2 += inc2;
  // 上位8bitを使ってテーブル参照
  //uint8_t s2 = pgm_read_byte(&SINE_TABLE[phase2 >> 24]); //フラッシュから参照したい場合
  uint8_t s1 = SINE_TABLE[phase1 >> 24];
  uint8_t s2 = SINE_TABLE[phase2 >> 24];
  // 合成・振幅調整
  uint8_t s1s = s1 >> 1;   // 0–127
  uint8_t s2s = s2 >> 1;   // 0–127
  uint8_t mix8 = s1s + s2s;      // 0–254

  uint16_t dacVal = (uint16_t)mix8 << 4;            // 12bitスケーリング
  sendToDAC(dacVal);
  }

void setup() {
  spi_init();

  // ── Timer2: CTC モード, prescaler=8
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A  = (F_CPU/8/SAMPLE_RATE) - 1;      // (16MHz/8)/16kHz - 1 = 124
  TIMSK2 |= _BV(OCIE2A);                  // 割り込み許可

  // フェーズインクリメントを計算 (32bit累算器向け)
  // phase += inc; で 32bit フルレンジ(2^32) を 1 サンプルで進む量
  inc1 = (uint32_t)((uint64_t)1000UL * 4294967296UL / SAMPLE_RATE);  // 1 kHz
  inc2 = (uint32_t)((uint64_t)2000UL * 4294967296UL / SAMPLE_RATE);  // 2 kHz
  sei();  // グローバル割り込み有効化
}

void loop() {
  // メインループは空。ISRで波形生成
}
