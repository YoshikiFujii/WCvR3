//1周期のsin波を32分割で再現
const uint8_t sineTable[32] = {
  128, 152, 176, 198, 218, 234, 245, 251,
  255, 251, 245, 234, 218, 198, 176, 152,
  128, 104, 80,  58,  38,  22,  11,   5,
    0,   5, 11,  22,  38,  58,  80, 104
};

//voatile→この変数は割り込みによって変更される可能性があると明示
//同時に発生させる周波数の数増やす
volatile uint8_t idx7000 = 0;
volatile uint8_t idxData = 0;
voltile uintt8_t dataCounter = 0;

// 周波数→OCR2Aの対応表（分周8、16MHz）
struct ToneSetting {
  uint16_t freq;
  uint8_t ocr2a;
};

ToneSetting tones[] = {
  {3000, 20},  // 3000Hz × 32 = 96kHz → OCR2A = 20
  {3500, 17},  // 4000Hz × 32 = 128kHz → OCR2A = 14
  {4000, 14},  // 5000Hz × 32 = 160kHz → OCR2A = 11
  {4500, 12}    // 6000Hz × 32 = 192kHz → OCR2A = 9
};
//同期用信号7kHz 7000Hz ✖️ 32 = 224kHz → OCR2A = 8

// 7000Hz ÷ 各toneの周波数 ≒ idxDataを何回に1回進めるか
const uint_8_t skipSteps[] = {7,6,5,4};

uint8_t currentTone = 0;
unsigned long lastSwitch = 0;
//fastPWMは分解能10bit　analogWriteなら8bit
//fastPWMについて、タイマーが０→最大値→０を高速に繰り返す間に、設定された比較値（OCRレジスタ）に達するとHIGH/LOWを切り替える
//これにより、ハードウェアPWM(タイマーとOCRレジスタ)が自動でデューティー比を計算してくれる
void setup() {
  pinMode(9, OUTPUT);
  // Timer1で62.5kHzのPWM（Fast PWMモード）
  //COM1A1:ピン９をPWM出力として有効化　WGM11 WGM10:波形の設定（fastPWM 10bit）
  TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10);
  //WGM12:上のFastPWMを継続する設定 CS10:クロック分周なし→PWM周波数は最大の62.5kHz
  TCCR1B = _BV(WGM12) | _BV(CS10);
  //例えばテーブルの値が128であれば128/1024=12.5%がデューティー比となる
  //割り込み発生前に初期値を設定する必要がある
  OCR1A = sineTable[0];

  // Timer2で10kHzの割り込み設定（割り込みを発生させる＝テーブルの一点の更新間隔を伸ばす）
  //WGM21:波形生成を、CTCモードに設定（カウンターがOCR2Aに達するとリセット→一定周期で割り込みを発生させる）
  TCCR2A = _BV(WGM21);
  //分周比(テーブルの一点の更新間隔を全て変える)を８にする→動作クロックは2MHz
  TCCR2B = _BV(CS21);
  OCR2A = 8; //同期用7000Hz信号（この信号は他の送信する信号よりも高周波にする）
  TIMSK2 = _BV(OCIE2A); // 割り込みのたびにISR()を実行
}

ISR(TIMER2_COMPA_vect) {
  idx7000 = (idx7000 + 1) % 32;
  dataCounter++;
  if(dataCounter >= skipSteps[currentTone]){
    dataCounter = 0;
    idxData = (idxData + 1) % 32;
  }
  uint16_t sum = sineTable[idxData] + sineTable[idx7000];
  OCR1A = sum / 2;
}

void loop() {
  //受信側が３回受信するのにかかる時間の間送信する。高速化できれば適宜変更する。
  if(millis() - lastSwitch > 200) {
    currentTone = (currentTone + 1) % 4;
    lastSwitch = millis();
  }
}
