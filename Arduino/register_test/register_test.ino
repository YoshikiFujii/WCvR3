void setup() {
  Serial.begin(115200);

  //入力チャンネルと基準電圧を選ぶ。基準電圧はAVcc、入力チャンネルはADC0 (A0)
  //ADMUXはADCの設定を行うためのレジスタ。
  //すなわちADMUX = 0b01000000;
  //これによって基準電圧：AVcc（=5v）入力チャンネル：ADC0（=A0ピン）ADLAR=0（結果は右詰め）
  ADMUX = (1 << REFS0);  // レジスタのビット６（REFS0だけを１にする

  // ADC制御設定
  // ADEN=1: ADC（アナログデジタル変換器）有効化,、ADSC=0: まだ変換開始しない
  // ADPS2:0=110: 分周率64 → ADCクロック = 16MHz/64 = 250kHz（Arduinoのクロックのまま動かすことは出来ないので、ADCの動作速度を設定する）
  //(1 << ADPS2) | (1 << ADPS1)によってADPS2~0が”011”になって以上の周波数に設定
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

void loop() {
  // 変換開始
  ADCSRA |= (1 << ADSC);
  // 変換完了を待つ（ADSCが0になるまで）
  while (ADCSRA & (1 << ADSC));
  // 結果取得（10ビット）
  int val = ADC;

  Serial.println(val);
  delayMicroseconds(100);  // 精度を見ながらサンプリング間隔を調整
}
