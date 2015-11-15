/*
 *  TankReceiver
 *  
 *  シリアル通信で受け取った値をモータードライバーに渡す
 *  Created 2014-03-23 by youkidearitai
 *  
 *  http://tekitoh-memdhoi.info/views/609
 */
#include <SoftwareSerial.h>
#include <stdlib.h>

// バッファのサイズ
#define BUFFERSIZE 128

// 送信パラメーターの左右のデリミタとして使用
// ex 411,233
#define DELIMITER ","

// 送信パラメーターの終了として使用
// ex 411,233CRLF
#define CR '\r'
#define LF '\n'

SoftwareSerial g_serial( 6, 7 );

// 入力待ちをするためのバッファ
char buffer[BUFFERSIZE];

/*
 * setup
 *
 * initialize pin
 * 3,5,10,11
 *
 * initialize serial
 */
void setup() {
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, INPUT); // Bluetooth ステータス用

  g_serial.begin(9600);
}

/*
 * シリアル通信で取得した値を読み込む
 * 0で取得中
 * 1で取得完了
 *
 * result 取得する値を格納する先
 * tmpStringSize バッファの最大サイズ
 */
int readCommandString(char result[], const int tmpStringSize) {
  static int index = 0;
  char c;
  char *tmp = &result[index];
  
  while ((c = g_serial.read()) != -1) {
    if (lostConnectStop()) {
      return 2;
    }

    switch (c) {
    case CR:
      break;
    case LF:
      *tmp++ = '\0';
      index = 0;
      return 1;
    default:
      if (index < tmpStringSize) {
        *tmp++ = c;
        index++;
      }
    }
  }
  return 0;
}

/*
 * シリアル通信で取得し終わったら捨てる
 */
void emptyBuffer() {
  while ((g_serial.read()) != -1);
}

/*
 * 取得した文字列を変換してモータードライバーへ送信するレベルを解析する
 *
 * result 取得結果(readCommandString関数のresult)
 * left 左側モーター
 * right 右側モーター
 */
void analyzeMotor(char result[], int &left, int &right) {

  char* tempLeft = strtok(result, DELIMITER);
  char* tempRight = strtok(NULL, DELIMITER);

  left = atoi(tempLeft);
  right = atoi(tempRight);

  left = betweenSpeed(left);
  right = betweenSpeed(right);
}

/*
 * 受信して、コマンド完了したことを通知する
 *
 * left 左側モーター
 * right 右側モーター
 */
void completeCommandWrite(int left, int right) {
  // response
  char response[16] = {'\0'};
  snprintf(response, 16, "%d,%d", left, right);
  g_serial.println(response);
}

/*
 * 0から511までの数値の範囲を返す
 * spd 0から511の範囲の値、0以下であれば0に511より上であれば511にする
 */
int betweenSpeed(int spd) {
  if (spd < 0) {
    return 0;
  }
  if (spd > 511) {
    return 511;
  }
  
  return spd;
}

/*
 * TA7291Pを操作する
 *
 * in1 in1への出力ピン
 * in2 in2への出力ピン
 *
 * spdの値
 * 511  正方向最速
 * 256  停止
 * 1    逆方向最速
 * 0    ブレーキ
 */
void cwOrCcw(int in1, int in2, int spd) {
  int cw = spd - 256;
 
  if (cw > 0) {
    analogWrite(in1, cw);
    analogWrite(in2, 0);
  } else if (cw == -256) {
    analogWrite(in1, 255);
    analogWrite(in2, 255);
  } else if (cw < 0) {
    analogWrite(in1, 0);
    analogWrite(in2, abs(cw));
  } else {
    // stop
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

/*
 * Bluetoothが接続されていないのならばモータードライバーの値を
 * stopに設定する
 */
int lostConnectStop() {
  if (digitalRead(12)) {
    return 0;
  }
  
  cwOrCcw(3, 5, 256);
  cwOrCcw(10, 11, 256);

  completeCommandWrite(256, 256);  
  return 1;
}


/*
 * main loop
 */
void loop() {
  int left = 0, right = 0;

  if (lostConnectStop()) {
    return;
  }

  if (readCommandString(buffer, BUFFERSIZE) != 1) {
    return;
  }
  
  emptyBuffer();
  cwOrCcw(3, 5, 256);
  cwOrCcw(10, 11, 256);
  
  delay(1);
  
  analyzeMotor(buffer, left, right);
  cwOrCcw(3, 5, left);
  cwOrCcw(10, 11, right);
  
  completeCommandWrite(left, right);
}
