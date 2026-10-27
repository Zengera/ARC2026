# Requirements.md

## プロジェクト概要

ESP32-WROOM-32E(Freenove ESP32-WROOM-32E) を使用した二輪ロボット制御ソフトウェアを開発する。

開発環境は以下とする。

- VSCode
- ESP-IDF
- C++
- ESP32-WROOM-32E

本ソフトウェアは以下3種類の入力方式によってロボットを制御する。

1. GPIO2接続の赤外線受信モジュール
2. GPIO4接続の赤外線受信モジュール
3. Bluetooth接続されたPS4コントローラ

入力ソースには優先順位があり、高優先度の入力が存在する場合は低優先度の入力を無視する。

優先順位は以下の通り。

| 優先度 | 入力 |
|----------|----------|
| 1 | GPIO2 赤外線受信モジュール |
| 2 | GPIO4 赤外線受信モジュール |
| 3 | PS4コントローラ |

# 使用ライブラリ

## PS4コントローラ

```cpp
#include <PS4Controller.h>
```

## 赤外線受信

```cpp
#include <IRremote.hpp>
```

# GPIO割り当て

## モータドライバ

### 左モータ

- GPIO18 : LEFT_A
- GPIO19 : LEFT_B

### 右モータ

- GPIO22 : RIGHT_A
- GPIO23 : RIGHT_B

## 赤外線受信機

- GPIO2 : IR_RX_HIGH
- GPIO4 : IR_RX_LOW

# モータ制御仕様

| A | B | 動作 |
|---|---|---|
| H | H | ショートブレーキ |
| H | L | 前進 |
| L | H | 後退 |
| L | L | 惰性回転 |

停止時はLLを使用する。

# PWM制御

- 周波数: 20kHz
- 分解能: 8bit
- デューティ: 0〜255

# PS4コントローラ仕様

- Bluetooth接続
- 左スティックY軸: 前後移動
- 右スティックX軸: 左右旋回
- 同時入力可能

ミキシング:

```cpp
leftMotor  = throttle + steering;
rightMotor = throttle - steering;
```

デッドゾーン:

```cpp
±15
```

シリアルモニターへ1秒ごとに入力値を表示する。

# 赤外線リモコン仕様

受信コードは16進数で表示する。

例:

```text
IR2 : 0x00FF18E7
IR4 : 0x00FF18E7
```

ボタン割り当て:

- 2 : 前進   0x02FD1FE0, 0x02FD0FF0
- 4 : 左旋回 0x02FD21DE, 0x02FD11EE
- 6 : 右旋回 0x02FD23DC, 0x02FD13EC
- 8 : 後退   0x02FD25DA, 0x02FD15EA
- 5 : 停止   0x02FD22DD, 0x02FD12ED, 0x02FD00FF

長押し中は動作継続。

150ms信号を受信しなければ停止する。

# 優先制御仕様

GPIO2 IR → GPIO4 IR → PS4 の順で優先する。

# フェイルセーフ

以下の場合は停止:

- 起動直後
- PS4未接続
- PS4切断
- IRタイムアウト
- 不正コマンド

停止状態:

```cpp
LEFT_A  = LOW;
LEFT_B  = LOW;
RIGHT_A = LOW;
RIGHT_B = LOW;
```

# 推奨クラス構成

- MotorController
- PS4Input
- IRInput
- RobotController
