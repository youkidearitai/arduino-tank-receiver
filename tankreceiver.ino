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

SoftwareSerial g_serial( 7, 6 );

char g_buffer[BUFFERSIZE] = ""; // バッファ
int  g_indexCounter = 0; // バッファを格納するときに使用するキー

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
 
  g_serial.begin(9600);
}

/*
 * シリアル通信で取得した値を読み込む
 * 0で取得中
 * 1で取得完了、resultに格納する
 *
 * index バッファのインデックス
 * tmpString バッファ
 * result 取得する値を格納する先
 * tmpStringSize バッファの最大サイズ
 */
int readCommandString(int &index, char tmpString[], char result[], const int tmpStringSize) {
  char c;  
  while ((c = g_serial.read()) != -1) {
    switch (c) {
    case '\r':
      tmpString[index++] = '\0';
      strncpy(result, tmpString, index + 1);
      index = 0;
      return 1;
    case '\n':
      break;
    default:
      if (index < tmpStringSize - 1) {
        tmpString[index++] = c;
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
 * main loop
 */
void loop() {
  char result[BUFFERSIZE];
  int left = 0, right = 0;

  if (!readCommandString(g_indexCounter, result, g_buffer, BUFFERSIZE)) {
    return;
  }
  
  emptyBuffer();
  delay(10);

  analyzeMotor(result, left, right);
  cwOrCcw(3, 5, left);
  cwOrCcw(10, 11, right);
  
  completeCommandWrite(left, right);
}
