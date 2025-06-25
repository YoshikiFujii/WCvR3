#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU        16000000UL
#define SAMPLE_RATE  16000UL
#define CS_PIN       PB2  // Arduino D10
#define MAX_TONES    4    // 合成する周波数数


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

volatile uint32_t phases[MAX_TONES] = {0};
uint32_t increments[MAX_TONES] = {0};

// SPIレジスタ操作による送信
void spi_init() {
  DDRB |= _BV(PB3) | _BV(PB5) | _BV(CS_PIN); // MOSI, SCK, CS
  SPCR = _BV(SPE) | _BV(MSTR);  // SPI有効化, マスター
  SPSR = _BV(SPI2X);            // 倍速（F_CPU/2）
}

void spi_send8(uint8_t data) {
  SPDR = data;
  while (!(SPSR & _BV(SPIF)));
}

void sendToDAC(uint16_t value12bit) {
  uint16_t cmd = 0b0011000000000000 | (value12bit & 0x0FFF);
  PORTB &= ~_BV(CS_PIN);        // CS LOW
  spi_send8(cmd >> 8);          // 上位8ビット
  spi_send8(cmd & 0xFF);        // 下位8ビット
  PORTB |= _BV(CS_PIN);         // CS HIGH
}

ISR(TIMER2_COMPA_vect) {
  uint16_t mix = 0;
  for (uint8_t i = 0; i < MAX_TONES; ++i) {
    phases[i] += increments[i];
    uint8_t s = SINE_TABLE[phases[i] >> 24];
    mix += ((uint16_t)(s + 1)) >> 1;  // 0〜127
  }

  // スケーリング：MAX_TONES × 127 → 0〜(127*N)を0〜4095に変換
  uint32_t scaled = ((uint32_t)mix * 4095) / (127 * MAX_TONES);
  sendToDAC((uint16_t)scaled);
}

void setup() {
  spi_init();
  
  // Timer2による割り込み16kHz
  TCCR2A = _BV(WGM21);   // CTCモード
  TCCR2B = _BV(CS21);    // 分周8
  OCR2A  = (F_CPU / 8 / SAMPLE_RATE) - 1;
  TIMSK2 |= _BV(OCIE2A);

  // 周波数を設定（例：1000Hz, 2000Hz, 3000Hz, 4000Hz）
  uint16_t freqs[MAX_TONES] = {1000, 2000, 3000, 4000};
  for (uint8_t i = 0; i < MAX_TONES; ++i) {
    increments[i] = (uint32_t)((uint64_t)freqs[i] * 4294967296UL / SAMPLE_RATE);
  }

  sei();
}

void loop() {
  // 何もしない（割り込みで動作）
}
