#include <arduinoFFT.h>

const uint16_t samples = 128; //サンプリング数　上げるほど分解能があがる
double samplingFrequency = 3000; //サンプリング周波数　上げるほど高周波を分解できる
double vReal[samples];
double vImag[samples];

ArduinoFFT<double> FFT = ArduinoFFT<double>();

void setup() {
  Serial.begin(115200);
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
  delay(500);
}

void loop() {
  unsigned long startMicros = micros();
  for (uint16_t i = 0; i<samples; i++){
    ADCSRA |= (1 << ADSC);  //ADC開始
    while (ADCSRA & (1 << ADSC)); //変換完了待ち
    int val = ADC;
    vReal[i] = val;
    vImag[i] = 0;
  }
  unsigned long elapsedMicros = micros() - startMicros;
  samplingFrequency = 1000000.0 * samples / elapsedMicros;  // μs → Hz
    // --- フーリエ変換 ---
  FFT.windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // ハミング窓　ピーク周波数のみ取り出す
  FFT.compute(vReal, vImag, samples, FFT_FORWARD);                  // FFT実行
  FFT.complexToMagnitude(vReal, vImag, samples);                    // 実部と虚部から振幅へ変換

  // --- 結果出力 ---
  for (uint16_t i = 3; i < samples / 2; i++) {
    // 出力される周波数の後半は鏡像になってるためsample/2
    //i=3までは直流部分（無音状態での電圧）で、大きな成分が出てしまうため。
    //samplingFrequency / sample で１ビンあたりの音の周波数間隔
    double freq = (i * samplingFrequency) / samples;
    Serial.print(freq, 1);
    Serial.print(" Hz : ");
    Serial.println(vReal[i], 1);  // 振幅
  }
  Serial.println("----");
  delay(100);  // 次のサイクルまで待つ
}
