#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU        16000000UL
#define SAMPLE_RATE  16000UL   // サンプル周波数 16 kHz
#define PWM_PIN      9         // OC1A (PB1)

//const uint8_t PROGMEM SINE_TABLE[256] = { //SRAMではなく、フラッシュにリストを保存する場合
const uint8_t SINE_TABLE[1024] = {
  128, 128, 129, 130, 131, 131, 132, 133, 134, 135, 135, 136, 137, 138, 138, 139,
  140, 141, 141, 142, 143, 144, 145, 145, 146, 147, 148, 148, 149, 150, 151, 152,
  152, 153, 154, 155, 155, 156, 157, 158, 158, 159, 160, 161, 161, 162, 163, 164,
  164, 165, 166, 167, 167, 168, 169, 170, 170, 171, 172, 172, 173, 174, 175, 175,
  176, 177, 178, 178, 179, 180, 180, 181, 182, 183, 183, 184, 185, 185, 186, 187,
  187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194, 195, 195, 196, 197, 197,
  198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206, 207, 207,
  208, 209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 215, 215, 216, 216, 217,
  217, 218, 218, 219, 219, 220, 221, 221, 222, 222, 223, 223, 224, 224, 225, 225,
  226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 230, 231, 231, 232, 232, 233,
  233, 234, 234, 234, 235, 235, 236, 236, 236, 237, 237, 238, 238, 238, 239, 239,
  240, 240, 240, 241, 241, 241, 242, 242, 242, 243, 243, 243, 244, 244, 244, 245,
  245, 245, 245, 246, 246, 246, 247, 247, 247, 247, 248, 248, 248, 248, 249, 249,
  249, 249, 249, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 252, 252, 252,
  252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254,
  254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
  255, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
  254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253, 252, 252, 252,
  252, 252, 252, 252, 251, 251, 251, 251, 251, 251, 250, 250, 250, 250, 249, 249,
  249, 249, 249, 248, 248, 248, 248, 247, 247, 247, 247, 246, 246, 246, 245, 245,
  245, 245, 244, 244, 244, 243, 243, 243, 242, 242, 242, 241, 241, 241, 240, 240,
  240, 239, 239, 238, 238, 238, 237, 237, 236, 236, 236, 235, 235, 234, 234, 234,
  233, 233, 232, 232, 231, 231, 230, 230, 230, 229, 229, 228, 228, 227, 227, 226,
  226, 225, 225, 224, 224, 223, 223, 222, 222, 221, 221, 220, 219, 219, 218, 218,
  217, 217, 216, 216, 215, 215, 214, 213, 213, 212, 212, 211, 210, 210, 209, 209,
  208, 207, 207, 206, 206, 205, 204, 204, 203, 203, 202, 201, 201, 200, 199, 199,
  198, 197, 197, 196, 195, 195, 194, 193, 193, 192, 191, 191, 190, 189, 189, 188,
  187, 187, 186, 185, 185, 184, 183, 183, 182, 181, 180, 180, 179, 178, 178, 177,
  176, 175, 175, 174, 173, 172, 172, 171, 170, 170, 169, 168, 167, 167, 166, 165,
  164, 164, 163, 162, 161, 161, 160, 159, 158, 158, 157, 156, 155, 155, 154, 153,
  152, 152, 151, 150, 149, 148, 148, 147, 146, 145, 145, 144, 143, 142, 141, 141,
  140, 139, 138, 138, 137, 136, 135, 135, 134, 133, 132, 131, 131, 130, 129, 128,
  128, 127, 126, 125, 124, 124, 123, 122, 121, 120, 120, 119, 118, 117, 117, 116,
  115, 114, 114, 113, 112, 111, 110, 110, 109, 108, 107, 107, 106, 105, 104, 103,
  103, 102, 101, 100, 100,  99,  98,  97,  97,  96,  95,  94,  94,  93,  92,  91,
   91,  90,  89,  88,  88,  87,  86,  85,  85,  84,  83,  83,  82,  81,  80,  80,
   79,  78,  77,  77,  76,  75,  75,  74,  73,  72,  72,  71,  70,  70,  69,  68,
   68,  67,  66,  66,  65,  64,  64,  63,  62,  62,  61,  60,  60,  59,  58,  58,
   57,  56,  56,  55,  54,  54,  53,  52,  52,  51,  51,  50,  49,  49,  48,  48,
   47,  46,  46,  45,  45,  44,  43,  43,  42,  42,  41,  40,  40,  39,  39,  38,
   38,  37,  37,  36,  36,  35,  34,  34,  33,  33,  32,  32,  31,  31,  30,  30,
   29,  29,  28,  28,  27,  27,  26,  26,  25,  25,  25,  24,  24,  23,  23,  22,
   22,  21,  21,  21,  20,  20,  19,  19,  19,  18,  18,  17,  17,  17,  16,  16,
   15,  15,  15,  14,  14,  14,  13,  13,  13,  12,  12,  12,  11,  11,  11,  10,
   10,  10,  10,   9,   9,   9,   8,   8,   8,   8,   7,   7,   7,   7,   6,   6,
    6,   6,   6,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,   3,   3,   3,
    3,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,
    3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
    6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   8,   9,   9,   9,  10,  10,
   10,  10,  11,  11,  11,  12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,
   15,  16,  16,  17,  17,  17,  18,  18,  19,  19,  19,  20,  20,  21,  21,  21,
   22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,  27,  27,  28,  28,  29,
   29,  30,  30,  31,  31,  32,  32,  33,  33,  34,  34,  35,  36,  36,  37,  37,
   38,  38,  39,  39,  40,  40,  41,  42,  42,  43,  43,  44,  45,  45,  46,  46,
   47,  48,  48,  49,  49,  50,  51,  51,  52,  52,  53,  54,  54,  55,  56,  56,
   57,  58,  58,  59,  60,  60,  61,  62,  62,  63,  64,  64,  65,  66,  66,  67,
   68,  68,  69,  70,  70,  71,  72,  72,  73,  74,  75,  75,  76,  77,  77,  78,
   79,  80,  80,  81,  82,  83,  83,  84,  85,  85,  86,  87,  88,  88,  89,  90,
   91,  91,  92,  93,  94,  94,  95,  96,  97,  97,  98,  99, 100, 100, 101, 102,
  103, 103, 104, 105, 106, 107, 107, 108, 109, 110, 110, 111, 112, 113, 114, 114,
  115, 116, 117, 117, 118, 119, 120, 120, 121, 122, 123, 124, 124, 125, 126, 127,
};

volatile uint32_t phase1 = 0, phase2 = 0;  // 位相位置（番目）
uint32_t inc1, inc2;                      // 一回の位相移動個数

ISR(TIMER2_COMPA_vect) {
  // フェーズを進める
  phase1 += inc1;
  phase2 += inc2;
  // 上位8bitを使ってテーブル参照
  //uint8_t s2 = pgm_read_byte(&SINE_TABLE[phase2 >> 24]); //フラッシュから参照したい場合
  uint8_t s1 = SINE_TABLE[phase1 >> 24];
  uint8_t s2 = SINE_TABLE[phase2 >> 24];
  // 合成・振幅調整
  uint8_t s1s = (s1 + 1) >> 1;   // 0–127
  uint8_t s2s = (s2 + 1) >> 1;   // 0–127
  uint8_t mix8 = s1s + s2s;      // 0–254
  OCR1A = s2s;
  }

void setup() {
  // PWMピンを出力に設定
  DDRB |= _BV(DDB1);  // digitalPin 9

  // ── Timer1: Fast PWM 10bit, non-inverting, prescaler=1
  TCCR1A = _BV(WGM11) | _BV(COM1A1);
  TCCR1B = _BV(WGM12) | _BV(CS10);

  // ── Timer2: CTC モード, prescaler=8
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A  = (F_CPU/8/SAMPLE_RATE) - 1;      // (16MHz/8)/16kHz - 1 = 124
  TIMSK2 |= _BV(OCIE2A);                  // 割り込み許可

  // フェーズインクリメントを計算 (32bit累算器向け)
  // phase += inc; で 32bit フルレンジ(2^32) を 1 サンプルで進む量
  inc1 = (uint32_t)((uint64_t)1000UL * 4294967296UL / SAMPLE_RATE);  // 1 kHz
  inc2 = (uint32_t)((uint64_t)7000UL * 4294967296UL / SAMPLE_RATE);  // 2 kHz
  sei();  // グローバル割り込み有効化
}

void loop() {
  // メインループは空。ISRで波形生成
}
