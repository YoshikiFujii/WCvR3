#define CONTROL_RATE 64      // Mozziの制御レート（制御用割り込み周波数）
#define AUDIO_RATE   32000   // Mozziのオーディオレート（サンプルレート）
#define NUM_OSC      2       // 同時に鳴らすsin波の数
#define DAC_CS_PIN   10      // MCP4921のCSピン

#include <MozziGuts.h>
#include <Oscil.h>               // DDSオシレータ
#include <tables/sin2048_int8.h> // 2048点のsin波テーブル
#include <SPI.h>

// 各周波数（Hz）を定義（int型に変更し、setFreq(int) を呼び出しやすく）
const int frequencies[NUM_OSC] = {1000, 2000};

// 複数のオシレータを定義（最大NUM_OSC個）
Oscil<2048, AUDIO_RATE> osc1(SIN2048_DATA);
Oscil<2048, AUDIO_RATE> osc2(SIN2048_DATA);

// MCP4921に12bitデータを送る関数
void sendToDAC(uint16_t val) {
  uint16_t cmd = 0b0011000000000000 | (val & 0x0FFF);
  digitalWrite(DAC_CS_PIN, LOW);
  SPI.transfer(cmd >> 8);
  SPI.transfer(cmd & 0xFF);
  digitalWrite(DAC_CS_PIN, HIGH);
}

void setup() {
  // SPIとCSピンの初期化
  pinMode(DAC_CS_PIN, OUTPUT);
  digitalWrite(DAC_CS_PIN, HIGH);
  SPI.begin();

  // Mozzi開始
  startMozzi(CONTROL_RATE);

  // 各オシレータの周波数設定（int型周波数を使ってsetFreq(int)を呼び出し）
  osc1.setFreq(frequencies[0]);
  osc2.setFreq(frequencies[1]);
}

// 1サンプル分の音声データを生成・出力
int updateAudio() {
  // 各オシレータからサンプル取得（-128 ～ +127）
  int s1 = osc1.next();
  int s2 = osc2.next();
  // 2波を平均して合成
  int mix = (s1 + s2) / 2;
  // -128～127 を 0～4095 にマッピングしてDACへ出力
  uint16_t dac_val = map(mix, -128, 127, 0, 4095);
  sendToDAC(dac_val);
  // MozziのPWM出力も同時に行う
  return mix;
}

// CONTROL_RATEごとに呼ばれる（今回は未使用）
void updateControl() {
}

void loop() {
  audioHook();  // Mozziの内部処理呼び出し
}
