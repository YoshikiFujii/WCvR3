//----------設定項目----------
uint16_t BASE_FREQ = 5000;                        // 同期信号の周波数(Hz)
uint16_t dataFreqs[] = {1000, 1500 ,2000 ,2500 ,3000, 3500, 4000, 4500};  // データ信号の周波数(Hz)
uint8_t targetCount = 8;                          // データ周波数の数
float noiseMultiple = 2.3;                        // ノイズ*noiseMultiple以上で信号が存在すると判定
const uint16_t samples = 600;                     // サンプル数
const uint8_t dividedSamples = 200; 
float samplingFrequency = 14000.0;                // サンプリング周波数(Hz)
//----------内部係数----------
float coeffs[9];                              // goertzelの一部。setupで算出
float magnitudes[9];                          // 周波数ごとの強度
float noiseLevels[9];                         // 各周波数ごとのノイズレベル
float syncThresholds[9];                      // noiselevel*noiseMultiple

const unsigned long intervalMicros = 1000000UL / 14000UL;

//-------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  //----------信号受信設定(レジスタへの書き込み)----------
  ADMUX = (1 << REFS0);                                 // AVccを基準電圧に
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);   // ADC有効化、クロック分周
  //----------goertzel事前計算----------
  for (int i = 0; i < targetCount + 1; i++) {
    float freq = (i < targetCount) ? dataFreqs[i] : BASE_FREQ;
    float k = 0.5 + ((samples * freq) / samplingFrequency);
    float omega = (2.0 * PI * k) / samples;
    coeffs[i] = 2.0 * cos(omega);
  }
  delay(1000); 
  //---------ノイズレベルの初期化---------
  for (int f = 0; f < targetCount + 1; f++) {
    float sumMagnitude = 0.0;
    for(int n = 0; n < 7; n++){
      float q0 = 0, q1 = 0, q2 = 0;
      for (uint16_t i = 0; i < samples; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        float val = ((float)ADC - 512.0);
        q0 = coeffs[f] * q1 - q2 + val;
        q2 = q1;
        q1 = q0;
      }
      sumMagnitude += sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeffs[f]);
    }
    noiseLevels[f] = sumMagnitude / 7.0;                               // 周波数ごとのノイズ振幅を保持
  }
  for (int f = 0; f < targetCount + 1; f++) {
    syncThresholds[f] = noiseLevels[f] * noiseMultiple;
  }
  //-----debug-----
  Serial.println("各周波数のノイズ:");
  for (int i = 0; i < targetCount + 1; i++) {
  Serial.print((i < targetCount) ? dataFreqs[i] : BASE_FREQ);
  Serial.print(" Hz: ");
  Serial.println(noiseLevels[i]);                                                   
  }
  delay(1000); 
  //---------------
}

void loop() {
  //--------------------------------------------------------------------
  //----------------------------Goertzel計算本体-------------------------
  float q0[targetCount+1] = {0}, q1[targetCount+1] = {0}, q2[targetCount+1] ={0};
  for(int i = 0 ; i < samples; i++){
    unsigned long startTime = micros();

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    float val = ((float)ADC - 512.0);
    for(int k=0; k<targetCount+1; k++){
      q0[k] = coeffs[k] * q1[k] - q2[k] + val;                 //式：q[n]=2cos(ω)⋅q[n−1]−q[n−2]+x[n]
      q2[k] = q1[k];
      q1[k] = q0[k];
    }
    while (micros() - startTime < intervalMicros);
  }
  for(int l=0; l<targetCount+1; l++){
    magnitudes[l] = sqrt(q1[l] * q1[l] + q2[l] * q2[l] - q1[l] * q2[l] * coeffs[l]);  // 振幅を計算
  }


  //--------------------------------------------------------------------
  //------------------------解析結果からデータ化---------------------------
  /*
  uint8_t catchedData = 0;                                        //最大7bit変数 増やせればuint16_tへ
  if (magnitudes[targetCount] >= syncThresholds[targetCount]) {   // 同期信号検出時のみ出力
    for (int i = 0; i < targetCount; i++) {
      if (magnitudes[i] >= syncThresholds[i]) {
        catchedData |= (1 << i);
      }
    }
    //----------コード出力----------
    Serial.print("catchedData(bin): ");
    for (int i = targetCount - 1; i >= 0; i--) {
      Serial.print((catchedData >> i) & 1);
    }
    Serial.println();
  }
  else{
    Serial.println("---");
  }
  */
  Serial.print(BASE_FREQ);
  Serial.print(" : ");
  Serial.println(magnitudes[targetCount]);
  for(int i=0; i<targetCount; i++){
    Serial.print(dataFreqs[i]);
    Serial.print(" : ");
    Serial.println(magnitudes[i]);
  }
}