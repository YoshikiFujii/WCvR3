#include <SPI.h>

#define SRAM_CS 10                                // チップセレクトピン（D10）
#define SRAM_WRITE 0x02
#define SRAM_READ  0x03
#define CPUFREQ 16000000
//----------設定項目----------
uint16_t BASE_FREQ = 5000;                        // 同期信号の周波数(Hz)
uint16_t dataFreqs[] = {1000, 1500 ,2000 ,2500 ,3000, 3500, 4000, 4500};  // データ信号の周波数(Hz)
uint8_t targetCount = 8;                          // データ周波数の数
uint8_t noiseCount = 5;                           //ノイズ計測回数
float noiseMultiple = 2.3;                        // ノイズ*noiseMultiple以上で信号が存在すると判定
const uint16_t totalSamplesNeeded = 500;          // サンプル数
uint16_t samplingFrequency = 14000;               // サンプリング周波数(Hz)
//----------内部係数----------
float coeffs[9];                              // goertzelの一部。setupで算出
float magnitudes[9];                          // 周波数ごとの強度
float noiseLevels[9];                         // 各周波数ごとのノイズレベル
float syncThresholds[9];                      // noiselevel*noiseMultiple
//--------グローバル変数--------
volatile int8_t adcBuffer[200];               // サンプリングデータ一時保管先   
volatile uint16_t bufferIndex = 0;            // ISR内サンプリングカウンタ
volatile bool writeReady = false;             // true: 外部SRAM書き込み可
volatile bool writeFirstHalf = true;          // true: 0〜99サンプル, false: 100〜199サンプル
uint16_t totalSamplesWritten = 0;             // 外部SRAMへの保管個数
uint8_t checkedNoiseCount = 0;                // ノイズ計測カウンタ



//-------------SRAM初期化-------------
void initSRAM() {
  pinMode(SRAM_CS, OUTPUT);
  digitalWrite(SRAM_CS, HIGH);
  SPI.begin();
}

//---外部SRAMに書き込む（val: 1バイト）---
void writeSRAM(uint16_t addr, int8_t val) {
  digitalWrite(SRAM_CS, LOW);
  SPI.transfer(SRAM_WRITE);
  SPI.transfer(highByte(addr));
  SPI.transfer(lowByte(addr));
  SPI.transfer((uint8_t)val);
  digitalWrite(SRAM_CS, HIGH);
}

//---------外部SRAMから読み込む---------
int8_t readSRAM(uint16_t addr) {
  digitalWrite(SRAM_CS, LOW);
  SPI.transfer(SRAM_READ);
  SPI.transfer(highByte(addr));
  SPI.transfer(lowByte(addr));
  int8_t val = (int8_t)SPI.transfer(0x00);
  digitalWrite(SRAM_CS, HIGH);
  return val;
}

//----------開始時のノイズ計測----------
void checkNoise() {
  for (uint16_t i = 0; i < targetCount + 1; i++) {
    noiseLevels[i] += magnitudes[i];
  }
  if (checkedNoiseCount == noiseCount - 1) {
    for (int f = 0; f < targetCount + 1; f++) {
      noiseLevels[f] = noiseLevels[f] / noiseCount;
      syncThresholds[f] = noiseLevels[f] * noiseMultiple;
    }
    //-----debug-----
    Serial.println("各周波数のノイズ:");
    for (int j = 0; j < targetCount + 1; j++) {
      Serial.print((j < targetCount) ? dataFreqs[j] : BASE_FREQ);
      Serial.print(" Hz: ");
      Serial.println(noiseLevels[j]);
    }
    delay(1000);   
  }
  checkedNoiseCount++;
}

//--------------------------------------------------------------------
//------------------------解析結果からデータ化---------------------------
void convertToData(){
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
  else {
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
  interrupts();         //次のサンプリング開始
}

//-------------------------------------------------------------------
//-----------割り込み処理：一回のgoertzel算出に使うサンプルを収集-----------
ISR(TIMER1_COMPA_vect) {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  int8_t val = (int8_t)(ADCH - 128);                // ADC(10bit) to 8bit

  adcBuffer[bufferIndex++] = val;
  if (bufferIndex == 100 || bufferIndex == 200) {
    writeReady = true;                              //　外部SRAMへの書き込み可
    writeFirstHalf = (bufferIndex == 100);          // 100ならバッファの前半が書き込み対象
  }
  if (bufferIndex >= 200){
    bufferIndex = 0;
  }
}

void setup() {
  Serial.begin(115200);
  initSRAM();
  //----------信号受信設定(レジスタへの書き込み)----------
  ADMUX = (1 << REFS0) | (1 << ADLAR);                    // AVcc基準、左詰めモード
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);     // ADC有効化、クロック分周
  //-------------goertzel事前計算-------------
  for (int i = 0; i < targetCount + 1; i++) {
    float freq = (i < targetCount) ? dataFreqs[i] : BASE_FREQ;
    float k = 0.5 + ((totalSamplesNeeded * freq) / (float)samplingFrequency);
    float omega = (2.0 * PI * k) / totalSamplesNeeded;
    coeffs[i] = 2.0 * cos(omega);
  }
  
  delay(100);                                           
  //----------Timer1 初期化（サンプリング周波数固定間隔）----------
  noInterrupts();                       // 全割り込み禁止
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);               // CTCモード
  TCCR1B |= (1 << CS10);                // プリスケーラ1（16MHz）
  OCR1A = CPUFREQ / samplingFrequency - 1;   // 14kHz
  TIMSK1 |= (1 << OCIE1A);              // ISRを呼ぶ
  interrupts();
}

void loop() {
  //---------------外部メモリへの書き込み---------------
  if (writeReady) {
    writeReady = false;
    uint8_t offset = writeFirstHalf ? 0 : 100;

    // SRAMに保存（アドレス：totalSamplesWritten〜）
    for (uint8_t i = 0; i < 100; i++) {
      writeSRAM(totalSamplesWritten + i, adcBuffer[offset + i]);
    }
    totalSamplesWritten += 100;
  }

  if (totalSamplesWritten >= totalSamplesNeeded) {
    noInterrupts();                                       // 割り込み一時停止
    bufferIndex = 0;                                      // 次回用にリセット
    totalSamplesWritten = 0;                              

    //----------Goertzel処理（SRAMから500サンプルを読む）----------
    for (int f = 0; f < targetCount + 1; f++) {
      float q0 = 0, q1 = 0, q2 = 0;
      for (uint16_t i = 0; i < totalSamplesNeeded; i++) {
        float val = (float)readSRAM(i);                   // アドレス0～499
        q0 = coeffs[f] * q1 - q2 + val;
        q2 = q1;
        q1 = q0;
      }
      magnitudes[f] = sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeffs[f]);
    }
    if (checkedNoiseCount < 5) {
      checkNoise();
    }
    else {
      convertToData();
    }
    interrupts();                         // 割り込み許可
  }
}