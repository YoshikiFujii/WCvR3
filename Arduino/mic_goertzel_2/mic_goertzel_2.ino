//----------設定項目----------
float BASE_FREQ = 7000; // 同期信号の周波数（Hz）
float dataFreqs[] = {3000, 3500, 4000, 4500};　// データ信号の周波数 (Hz)
uint8_t targetCount = 4; //データ周波数の数
float noiseMultiple = 1.5; //ノイズ*noiseMultiple以上で信号が存在すると判定
//---------デバイス依存--------
const uint16_t samples = 128;
float samplingFrequency = 3000.0; // Hz
//----------内部係数----------
float coeffs[targetCount + 1];
float magnitudes[targetCount + 1];
float noiseSize;
//データがあるとみなす振幅の大きさ
float syncThreshold;
float dataThreshold;
//loop()内の一回のみの処理（サンプリング周波数更新とノイズ測定）の実行済判定
bool initialized = false; 

//----------信号受信設定(レジスタへの書き込み)----------
void setup() {
  Serial.begin(115200);
  ADMUX = (1 << REFS0);  // AVccを基準電圧に
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);  // ADC有効化、クロック分周
  delay(500);
}

void loop() {
  //-----------開始前に、サンプリングにかかる時間からサンプリング周波数を更新する-----------
  if(!initialized){
    float q0=0, q1=0, q2=0;
    unsigned long startMicros = micros();
    for (uint16_t i = 0; i < samples; i++) {
      // ADC読み取り
      ADCSRA |= (1 << ADSC);
      while (ADCSRA & (1 << ADSC));
      float val = (float)ADC;

      // Goertzelアルゴリズム本体
      q0 = 0.0 * q1 - q2 + val;
      q2 = q1;
      q1 = q0;
    }
    
    unsigned long elapsedMicros = micros() - startMicros;
    float timePerSample = (float)elapsedMicros / samples;
    samplingFrequency = 1000000.0 / timePerSample * 0.9; //安全のため、90%のマージン
    Serial.print("推定サンプリング周波数："); //debug
    Serial.print(samplingFrequency, 2); //debug
    Serial.println("Hz"); //debug

    //----------各周波数のノイズの平均値を測定----------
    for(int i=0; i<targetCount + 1; i++){
      float k;
      if (i < targetCount)
        k = 0.5 + ((samples * dataFreqs[i]) / samplingFrequency);
      else
        k = 0.5 + ((samples * BASE_FREQ) / samplingFrequency);

      float omega = (2.0 * PI * k) / samples;
      coeffs[i] = 2.0 * cos(omega);
    }
    for (int f = 0; f < targetCount+1; f++) {
      float q0 = 0, q1 = 0, q2 = 0;
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
      // 振幅を計算
      magnitudes[f] = sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeffs[f]);
    }
    float sum=0;
    for(int i=0; i<targetCount+1; i++)
      sum += magnitudes[i];
    noiseSize = sum / (targetCount + 1);
    syncThreshold = noiseSize * noiseMultiple;
    dataThreshold = noiseSize * noiseMultiple;
    initialized = true;
  }
  //--------------------------------------------------------------------
  //----------------------------Goertzel計算本体-------------------------
  // 各周波数ごとに初期化
  unsigned long startGtl = micros(); //debug
  for (int f = 0; f < targetCount+1; f++) {
    float q0 = 0, q1 = 0, q2 = 0;

    for (uint16_t i = 0; i < samples; i++) {
      // ADC読み取り
      ADCSRA |= (1 << ADSC);
      while (ADCSRA & (1 << ADSC));
      float val = (float)ADC;

      //式：q[n]=2cos(ω)⋅q[n−1]−q[n−2]+x[n]
      q0 = coeffs[f] * q1 - q2 + val;
      q2 = q1;
      q1 = q0;
    }
    // 振幅を計算
    magnitudes[f] = sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeffs[f]);
  }

  //--------------------------------------------------------------------
  //------------------------解析結果からデータ化---------------------------
  uint8_t catchedData = 0; //最大7bit変数 増やせればuint16_tへ
  if(magnitudes[targetCount] >= syncThreshold){ //同期信号がある時のみ受信
    for(int i=0; i<targetCount; i++){
      if(magnitudes[i] >= dataThreshold){
        catchedData |= (1 << i);  // ビット立て
      } 
    }
    //----------コード出力----------
    Serial.print("catchedData(bin): ");
    for (int i = targetCount - 1; i >= 0; i--) {
      Serial.print((catchedData >> i) & 1);
    }
    Serial.println();
  }
  Serial.print("一回の受信時間："); //debug
  Serial.println(micros() - startGtl); //debug
}