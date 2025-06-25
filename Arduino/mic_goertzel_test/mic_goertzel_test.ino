//----------設定項目----------
uint16_t BASE_FREQ = 5000;                        // 同期信号の周波数(Hz)
uint16_t dataFreqs[] = {1000, 1500 ,2000 ,2500 ,3000, 3500, 4000, 4500};  // データ信号の周波数(Hz)
uint8_t targetCount = 8;                          // データ周波数の数
float noiseMultiple = 2.5;                        // ノイズ*noiseMultiple以上で信号が存在すると判定
const uint16_t samples = 210;                     // サンプル数
float samplingFrequency = 14000.0;                // サンプリング周波数(Hz)
//----------内部係数----------
#define BUFFER_SIZE 200
#define WINDOW_SIZE 600

float coeffs[9];                              // goertzelの一部。setupで算出
float magnitudes[9];                          // 周波数ごとの強度
float noiseLevels[9];                         // 各周波数ごとのノイズレベル
float syncThresholds[9];                      // noiselevel*noiseMultiple
//--- Timer用グローバル変数 ---
volatile uint16_t sampleIndex = 0;            // ISR内カウント用

int16_t adcBuffer[BUFFER_SIZE];                   // リングバッファとして使用
volatile uint16_t writeIndex = 0;
uint16_t readIndex = 0;

//-------------------------------------------------------------------
//-----------割り込み処理：一回のgoertzel算出に使うサンプルを収集-----------
ISR(TIMER1_COMPA_vect) {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  int16_t val = (int16_t)ADC - 512;   //ADC読み取り
  for(int f=0; f<targetCount+1; f++){
    float q0 = coeffs[f]*q1[f] - q2[f] + val;
    q2[f] = q1[f];
    q1[f] = q0;
  }
  sampleCount++;
  adcBuffer[writeIndex % BUFFER_SIZE] = val;   //バッファに書き込み
  writeIndex++;
}

void setup() {
  Serial.begin(115200);
  //----------信号受信設定(レジスタへの書き込み)----------
  ADMUX = (1 << REFS0);                                 // AVccを基準電圧に
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);   // ADC有効化、クロック分周
  //----------goertzel事前計算----------
  for (int i = 0; i < targetCount + 1; i++) {
    float freq = (i < targetCount) ? dataFreqs[i] : BASE_FREQ;
    float k = 0.5 + ((WINDOW_SIZE * freq) / samplingFrequency);
    float omega = (2.0 * PI * k) / WINDOW_SIZE;
    coeffs[i] = 2.0 * cos(omega);
  }
  
  delay(1000); 
  //---------ノイズレベルの初期化---------
  for (int f = 0; f < targetCount + 1; f++) {
    float sumMagnitude = 0.0;
    for(int n = 0; n < 7; n++){
      float nq0 = 0, nq1 = 0, nq2 = 0;
      for (uint16_t i = 0; i < samples; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        float val = (float)ADC - 512.0;
        nq0 = coeffs[f] * nq1 - nq2 + val;
        nq2 = nq1;
        nq1 = nq0;
      }
      sumMagnitude += sqrt(nq1 * nq1 + nq2 * nq2 - nq1 * nq2 * coeffs[f]);
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
  delay(1000);                                                         //これがないと何故か先にloop()が動き始める
  //---------------
  //----------Timer1 初期化（14kHz固定間隔）----------
  noInterrupts();                       // 全割り込み禁止
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);               // CTCモード
  TCCR1B |= (1 << CS10);                // プリスケーラ1（16MHz）
  OCR1A = 16000000 / 14000 - 1;         // 14kHz
  TIMSK1 |= (1 << OCIE1A);              // ISRを呼ぶ
  interrupts();                         // 割り込み許可
}

void loop() {
  if (readIndex < WINDOW_SIZE - 1) return;
  noInterrupts(); 

  for (int f = 0; f < targetCount+1; f++) {           // 各周波数ごとに初期化
    magnitudes[f] = sqrt(q1[f] * q1[f] + q2[f] * q2[f] - q1[f] * q2[f] * coeffs[f]);  // 振幅を計算
  }

  //--------------------------------------------------------------------
  //------------------------解析結果からデータ化---------------------------
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
  /*
  Serial.print(BASE_FREQ);
  Serial.print(" : ");
  Serial.println(magnitudes[targetCount]);
  for(int i=0; i<targetCount; i++){
    Serial.print(dataFreqs[i]);
    Serial.print(" : ");
    Serial.println(magnitudes[i]);
  }
  */
  readIndex = 0;
  writeIndex = 0;
  interrupts();         //次のサンプリング開始
}