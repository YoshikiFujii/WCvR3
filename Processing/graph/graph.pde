import processing.serial.*;

Serial port;

ArrayList<Float> freqs = new ArrayList<Float>();
ArrayList<Float> amps = new ArrayList<Float>();
boolean dataUpdated = false;

void setup() {
  size(900, 400);
  port = new Serial(this, "/dev/tty.usbmodem1301", 115200);  // ポート番号は必要に応じて変更
  port.bufferUntil('\n');  // 行単位で受信
}

void draw() {
  if (dataUpdated) {
    background(0);  // ← データが届いたときだけ消す
    drawGraph();
    dataUpdated = false;
  }
}

void drawGraph(){
    // --- グリッドと横軸の目盛り（周波数ラベル） ---
  stroke(100);
  fill(200);
  textSize(10);
  for (int f = 0; f <= 8000; f += 500) {  // 100Hzごとにラベル
    float x = map(f, 0, 8000, 0, width);
    line(x, 0, x, height);
    textAlign(CENTER);
    text(f + "Hz", x, height - 5);
  }
  
  stroke(0, 255, 0);
  noFill();
  beginShape();
  for (int i = 0; i < freqs.size(); i++) {
    float x = map(freqs.get(i), 0, 8000, 0, width);   // 横軸：周波数 0〜5kHz
    float y = map(amps.get(i), 0, 500, height, 0);    // 縦軸：振幅（100は調整可）
    vertex(x, y);
  }
  endShape();
  
  fill(255);
  text("FFT Spectrum", 10, 20);
  
  freqs.clear();
  amps.clear();
}

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);

  if (line.equals("----")) {
    dataUpdated = true;
    // 1セット終わったら画面更新（次の draw() へ）
    return;
  }

  try {
    String[] parts = split(line, "Hz :");
    if (parts.length == 2) {
      float freq = float(trim(parts[0]));
      float amp = float(trim(parts[1]));
      if (!Float.isNaN(freq) && !Float.isNaN(amp)) {
        freqs.add(freq);
        amps.add(amp);
      }
    }
  } catch (Exception e) {
    println("Parse error: " + line);
  }

  // データ数が多すぎる場合はリセット（前の分を消す）
  if (freqs.size() > 64) {
    freqs.clear();
    amps.clear();
  }
}
void keyPressed() {
  if (key == 's' || key == 'S') {
    String timestamp = nf(year(), 4) + nf(month(), 2) + nf(day(), 2) + "_" +
                       nf(hour(), 2) + nf(minute(), 2) + nf(second(), 2);
    saveFrame("screenshot_" + timestamp + ".png");
    println("スクリーンショットを保存しました: screenshot_" + timestamp + ".png");
  }
}
