# Architecture.md

## システム構成

本システムは入力処理、制御判定、モータ制御を明確に分離する。

```text
main.cpp
    │
    ▼
RobotController
    ├─ PS4Input
    ├─ IRReceiver2
    ├─ IRReceiver4
    └─ MotorController
```

## 各クラスの責務

### RobotController

システム全体の制御を行う。

責務

- 各入力デバイスの状態取得
- 入力優先順位判定
- モーション生成
- MotorControllerへの出力指示
- フェイルセーフ制御

RobotControllerのみが制御判断を行う。

---

### PS4Input

PS4コントローラを管理する。

責務

- Bluetooth接続
- 接続状態取得
- スティック入力取得
- ボタン状態取得
- 定期ログ出力

取得する情報

```cpp
int throttle;
int steering;

bool connected;
bool brakeButton;
```

---

### IRReceiver2

GPIO2接続赤外線受信機を管理する。

責務

- IR信号受信
- コマンド変換
- タイムアウト管理
- ログ出力

優先順位

```text
最高優先
```

---

### IRReceiver4

GPIO4接続赤外線受信機を管理する。

責務

- IR信号受信
- コマンド変換
- タイムアウト管理
- ログ出力

優先順位

```text
IRReceiver2より低い
PS4より高い
```

---

### MotorController

モータドライバを管理する。

責務

- GPIO初期化
- PWM生成
- 前進
- 後退
- 左旋回
- 右旋回
- 停止
- ショートブレーキ

---

## 制御周期

```text
10ms
```

RobotControllerは10msごとに実行される。

---

## 実行シーケンス

```text
readInputs()
    ↓
determineControlSource()
    ↓
generateMotionCommand()
    ↓
applyMotorOutput()
```

---

## 入力優先順位

```text
IRReceiver2
    ↓
IRReceiver4
    ↓
PS4Input
```

同時入力時は高優先度を採用する。

GPIO2およびGPIO4が同時入力された場合はGPIO2を採用する。

---

## 制御ソース状態

```cpp
enum class ControlSource
{
    NONE,
    IR2,
    IR4,
    PS4
};
```

---

## IRタイムアウト処理

IR信号受信時は制御権を取得する。

信号が途絶えてから

```text
150ms
```

を経過したらタイムアウトとみなす。

さらに

```text
500ms
```

経過後にPS4制御へ復帰する。

---

## モーションコマンド

```cpp
enum class MotionCommand
{
    COAST_STOP,
    BRAKE,
    FORWARD,
    BACKWARD,
    TURN_LEFT,
    TURN_RIGHT,
    MIXED_DRIVE
};
```

---

## PS4ミキシング

```cpp
leftMotor  = throttle + steering;
rightMotor = throttle - steering;
```

出力範囲

```text
-255 ～ +255
```

を超えた場合は比率維持スケーリングを行う。

例

```text
300,100
↓
255,85
```

---

## 停止およびブレーキ

通常停止

```text
LL
```

異常停止

```text
LL
```

ショートブレーキ

```text
HH
```

ショートブレーキ入力

```text
PS4 R1ボタン
IRリモコン 5ボタン
```

---

## Bluetooth

起動時にESP32のBluetooth MACアドレスをシリアルへ出力する。

例

```text
System Start
ESP32 Bluetooth MAC:
AA:BB:CC:DD:EE:FF
```

接続状態も出力する。

```text
PS4 Connected
PS4 Disconnected
```

---

## フェイルセーフ

以下の場合は停止する。

- 起動直後
- PS4切断
- 不正IRコード
- 初期化失敗

停止方式

```text
LL
```

---

## ログ出力

ログレベル

```text
Normal
```

### IR受信

```text
IR2 : 0xXXXXXXXX
IR4 : 0xXXXXXXXX
```

### 制御ソース変更

```text
Control Source : IR2
Control Source : IR4
Control Source : PS4
```

### 定期出力

1秒ごと

```text
PS4:
LY=xxx
RX=xxx

Motor:
L=xxx
R=xxx
```

IR制御中であってもPS4入力は取得・表示を継続する。

---

## main.cppの方針

main.cppには制御ロジックを書かない。

```cpp
RobotController robot;

void setup()
{
    robot.begin();
}

void loop()
{
    robot.update();
}
```

すべての制御ロジックはクラスへ集約する。
