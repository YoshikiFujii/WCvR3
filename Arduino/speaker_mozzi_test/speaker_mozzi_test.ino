#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

// updateControl()の呼び出し間隔を指定 デフォルトは64
#define CONTROL_RATE 128

// オシレーターの初期化
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);


void setup(){
    startMozzi(CONTROL_RATE);
    aSin.setFreq(5000);
}

// 使わない場合も必須
void updateControl(){
}


int updateAudio(){
    return aSin.next();
}

void loop(){
    // 実際に音の合成を行う関数
    audioHook();
}