import processing.serial.*;

Serial myPort;

int freqCount = 9;
int timeSteps = 200;  // 横幅方向の履歴数
float[][] history = new float[freqCount][timeSteps];  // [周波数][時間]
String[] freqLabels = { "1000", "1500", "2000", "2500","3000", "3500", "4000", "4500", "5000" };
color[] freqColors = { color(255,0,0), color(0,255,0), color(0,0,255), color(255,0,128), color(255,128,0), color(0,255,128), color(128,255,0), color(0,128,255), color(128,0,255)};

float[] latestValues = new float[freqCount];
String inputLine = "";

void setup() {
  size(800, 400);
  println(Serial.list());
  myPort = new Serial(this, "/dev/cu.usbmodem1401", 115200);
  myPort.bufferUntil('\n');
  background(0);
}

void draw() {
  background(0);

  // 折れ線グラフの描画
  for (int f = 0; f < freqCount; f++) {
    stroke(freqColors[f]);
    noFill();
    beginShape();
    for (int t = 0; t < timeSteps; t++) {
      float val = history[f][t];
      float x = map(t, 0, timeSteps - 1, 50, width - 10);
      float y = map(val, 0, 3000, height - 20, 20);
      vertex(x, y);
    }
    endShape();

    fill(freqColors[f]);
    text(freqLabels[f] + " Hz", 5, 20 + f * 15);
  }

  // マウス位置に対応する時間インデックスを取得
  int hoverT = int(map(mouseX, 50, width - 10, 0, timeSteps - 1));
  hoverT = constrain(hoverT, 0, timeSteps - 1);

  // 垂直線を描画
  stroke(200);
  float hoverX = map(hoverT, 0, timeSteps - 1, 50, width - 10);
  line(hoverX, 0, hoverX, height);

  // ホバー情報の表示
  fill(255);
  textAlign(LEFT);
  float yOffset = 20;
  for (int f = 0; f < freqCount; f++) {
    float val = history[f][hoverT];
    text(freqLabels[f] + ": " + nf(val, 1, 2), 60, yOffset + f * 15);
  }
}


void serialEvent(Serial p) {
  inputLine = trim(p.readStringUntil('\n'));
  if (inputLine == null || inputLine.length() == 0) return;

  if (inputLine.contains(":")) {
    String[] parts = split(inputLine, ':');
    if (parts.length == 2) {
      String freqStr = trim(parts[0]);
      String valStr = trim(parts[1]);
      float value = float(valStr);
      for (int i = 0; i < freqCount; i++) {
        if (freqLabels[i].equals(freqStr)) {
          latestValues[i] = value;
        }
      }
    }
  }

  // 7000Hz（同期信号）を受け取ったら1フレーム分更新
  if (inputLine.startsWith("5000")) {
    for (int i = 0; i < freqCount; i++) {
      // 時系列データを左にずらす
      for (int j = 0; j < timeSteps - 1; j++) {
        history[i][j] = history[i][j + 1];
      }
      // 最新値を格納
      history[i][timeSteps - 1] = latestValues[i];
    }
  }
}
