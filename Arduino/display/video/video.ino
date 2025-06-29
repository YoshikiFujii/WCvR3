#include <TVout.h>
#include <fontALL.h>
#include <SoftwareSerial.h>

TVout TV;
SoftwareSerial mySerial(10, 11); // RX, TX（送信側Arduinoと接続）

const uint8_t maxLines = 12; // TV画面に収まる最大行数（96ピクセル ÷ フォント高さ8px = 12行）
String lines[maxLines];      // 表示履歴
String buffer = "";          // 受信中の1行
bool updated = false;

void setup() {
  TV.begin(_NTSC, 128, 96); // NTSC, 解像度（PALなら_PAL）
  TV.select_font(font6x8);
  TV.println("Waiting for data...");

  mySerial.begin(9600); // 送信側と一致させる
}

void loop() {
  // シリアル受信（1行単位で処理）
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      updated = true;
      break;
    } else {
      buffer += c;
    }
  }

  // 1行受信完了したらログを更新
  if (updated) {
    // 1行ずつ繰り上げ
    for (int i = 0; i < maxLines - 1; i++) {
      lines[i] = lines[i + 1];
    }
    lines[maxLines - 1] = buffer; // 最下行に新しいデータを追加
    buffer = "";
    updated = false;

    // 表示更新
    TV.clear_screen();
    for (int i = 0; i < maxLines; i++) {
      TV.set_cursor(0, i * 8); // フォント高さに応じてy位置
      TV.println(lines[i]);
    }
  }
}
