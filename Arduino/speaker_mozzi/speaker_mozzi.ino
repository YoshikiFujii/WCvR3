// この3つは必須
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <LowPassFilter.h>  // ローパスフィルタ追加

// updateControl()の呼び出し間隔を指定
#define CONTROL_RATE 128

// オシレーターの初期化
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
// 振幅補正ゲイン（例）
const float gain_list[] = {1.0, 1.2, 1.5, 1.8, 2.0, 2.5, 3.0}; 
// ローパスフィルタの初期化
LowPassFilter filter;

// 周波数リスト
const int freq_list[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000};
const int freq_count = sizeof(freq_list) / sizeof(freq_list[0]);
int current_index = 0;

// カウンタ変数
unsigned long count = 0;
const unsigned long change_interval = CONTROL_RATE * 1;  // 1秒ごとに変更

void setup(){
    startMozzi(CONTROL_RATE);
    aSin.setFreq(freq_list[current_index]);

    // カットオフ周波数は出したい音より少し高めに設定
    filter.setCutoffFreq(8000);
}

void updateControl(){
    count++;
    if (count >= change_interval) {
        count = 0;
        // 次の周波数へ
        current_index++;
        if (current_index >= freq_count) {
            current_index = 0;
        }
        aSin.setFreq(freq_list[current_index]);
    }
}

int updateAudio(){
    int32_t sig = aSin.next();
    // 振幅補正
    sig = (int32_t)(sig * gain_list[current_index]);

    // ローパスフィルタを適用
    sig = filter.next(sig);

    // clip to [-128,127]
    if(sig > 127) sig = 127;
    if(sig < -128) sig = -128;
    return sig;
}

void loop(){
    audioHook();
}
