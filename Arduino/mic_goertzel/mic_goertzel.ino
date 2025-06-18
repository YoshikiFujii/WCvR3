//---------デバイス依存--------
const uint16_t samples = 128;
float samplingFrequency = 15000.0; // Hz

//----------設定項目----------
uint8_t targetCount = 4;
// 対象周波数（Hz）
float targetFrequencies[] = {3000, 4000, 5000, 6000};

//----------内部係数----------
float coeffs[4];
float magnitudes[4];

void setup() {
  Serial.begin(115200);
  ADMUX = (1 << REFS0);  // AVccを基準電圧に
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);  // ADC有効化、クロック分周
  delay(500);

  // Goertzel係数を事前計算（cos(2πk/N)）
  for (int i = 0; i < targetCount; i++) {
    //k:周波数ごとのbinのインデックス
    //一回で行うFFTの結果の中から、ターゲット周波数がどのbinに当たるかを計算
    float k = 0.5 + ((samples * targetFrequencies[i]) / samplingFrequency);  
    //目標周波数に対応する角周波数
    float omega = (2.0 * PI * k) / samples;
    coeffs[i] = 2.0 * cos(omega);
  }
}

void loop() {
  //式：q[n]=2cos(ω)⋅q[n−1]−q[n−2]+x[n]
  // Goertzel変数
  float q0, q1, q2;
  
  // 各周波数ごとに初期化
  for (int f = 0; f < targetCount; f++) {
    q0 = q1 = q2 = 0.0;
    unsigned long startMicros = micros();
    for (uint16_t i = 0; i < samples; i++) {
      // ADC読み取り
      ADCSRA |= (1 << ADSC);
      while (ADCSRA & (1 << ADSC));
      float val = (float)ADC;

      // Goertzelアルゴリズム本体
      q0 = coeffs[f] * q1 - q2 + val;
      q2 = q1;
      q1 = q0;
    }
    unsigned long elapsedMicros = micros() - startMicros;
    float timePerSample = (float)elapsedMicros / samples;
    samplingFrequency = 1000000.0 / timePerSample * 0.9; //安全のため、90%のマージン

    // 振幅を計算
    magnitudes[f] = sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeffs[f]);
  }

  // --- 最大振幅の周波数を特定 ---
  int maxIndex = 0;
  float maxVal = magnitudes[0];
  for (int i = 1; i < targetCount; i++) {
    if (magnitudes[i] > maxVal) {
      maxVal = magnitudes[i];
      maxIndex = i;
    }
  }
  // --- コード出力 ---
  switch (maxIndex) {
    case 0: Serial.println("00"); break;  // 3kHz
    case 1: Serial.println("01"); break;  // 4kHz
    case 2: Serial.println("10"); break;  // 5kHz
    case 3: Serial.println("11"); break;  // 6kHz
  }
//  delay(100);
}
