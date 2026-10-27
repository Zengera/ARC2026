# ARC2026 - ESP32 二輪ロボット制御ソフトウェア

ESP32-WROOM-32E (Freenove ESP32-WROOM-32E) を搭載した二輪独立駆動ロボット向けの制御ファームウェアです。  
赤外線受信モジュール（2系統）および Bluetooth接続された PS4コントローラ による複合入力制御に対応し、厳密な優先制御とフェイルセーフ機能を備えています。

---

## 主な機能と特徴

- **優先順位ベースの入力切り替え**  
  複数の入力ソースが同時に存在する場合でも、優先順位に従って安全かつ決定論的に制御ソースを自動切り替えします。
  1. **IR2 (GPIO 2接続 赤外線受信モジュール)**【最優先】
  2. **IR4 (GPIO 4接続 赤外線受信モジュール)**【優先度2】
  3. **PS4コントローラ (Bluetooth)**【優先度3】
- **PS4コントローラによる滑らかなアナログ操舵**  
  - 左スティック前後 (LY) で前進・後退 (Throttle)、右スティック左右 (RX) で旋回 (Steering) をミキシング。
  - スティックの不感帯（デッドゾーン ±15）処理。
  - 最大出力（±255）超過時の**比率維持スケーリング**（例: 300, 100 → 255, 85）。
  - R1ボタンによる即時ショートブレーキ機能。
- **2系統の赤外線リモコン受信 (NECフォーマット)**  
  - テンキー（2: 前進, 4: 左旋回, 5: ショートブレーキ, 6: 右旋回, 8: 後退）操作に対応。
  - 長押し（リピートコード）による連続走行対応。
  - 受信コードの16進数リアルタイム表示。
- **高信頼なフェイルセーフ＆タイムアウト設計**  
  - 150ms 以上 IR 信号を受信しなかった場合の自動惰性停止 (LL)。
  - 途絶から 650ms (150ms + 500ms) 経過後の PS4 制御権自動復帰。
  - 起動直後、PS4 切断時、不正 IR コード受信時の安全停止。
- **10ms (100Hz) 周期の確定的な制御ループ**  
  - `readInputs()` → `determineControlSource()` → `generateMotionCommand()` → `applyMotorOutput()` の明確なパイプライン。
- **リアルタイムシリアルモニタリング (115200 bps)**  
  - 起動時の Bluetooth MAC アドレス出力。
  - 制御ソース切り替え通知、1秒周期のスティック値およびモータ出力デューティ出力。

---

## ハードウェア仕様・GPIO割り当て

### モータドライバ (Hブリッジ / PWM制御)

PWM周波数 **20kHz**、分解能 **8bit (0〜255)** にて制御します。

| 部位 | 信号名 | GPIO | 備考 |
|---|---|---|---|
| **左モータ** | LEFT_A | GPIO 18 | PWM出力 (前進時PWM / 後退時LOW) |
| | LEFT_B | GPIO 19 | PWM出力 (前進時LOW / 後退時PWM) |
| **右モータ** | RIGHT_A | GPIO 22 | PWM出力 (前進時PWM / 後退時LOW) |
| | RIGHT_B | GPIO 23 | PWM出力 (前進時LOW / 後退時PWM) |

#### モータ真理値表

| A | B | モータ動作 | 備考 |
|:---:|:---:|---|---|
| **H** | **H** | **ショートブレーキ** | PS4 R1ボタン / IR 5ボタン |
| **H** | **L** | **前進** | 前進回転 |
| **L** | **H** | **後退** | 後退回転 |
| **L** | **L** | **惰性回転 (停止)** | 通常停止・フェイルセーフ停止時 |

### 赤外線受信モジュール

| 名称 | GPIO | 優先度 |
|---|---|---|
| **IR2 (IR_RX_HIGH)** | GPIO 2 | 優先度 1 (最優先) |
| **IR4 (IR_RX_LOW)** | GPIO 4 | 優先度 2 |

---

## ソフトウェアアーキテクチャ

オブジェクト指向設計により、入力処理・制御判断・モータ制御を完全に分離しています。`main.cpp` には制御ロジックを置かず、クラスへ集約されています。

```text
main.cpp
    │
    ▼
RobotController
    ├── MotorController (モータGPIO / PWM / ブレーキ管理)
    ├── PS4Input        (Bluetooth接続 / スティック・ボタン処理)
    ├── IRReceiver (IR2: GPIO 2 管理)
    └── IRReceiver (IR4: GPIO 4 管理)
```

### 各モジュールの責務

| クラス / ファイル | 責務 |
|---|---|
| **`RobotController`** | システム全体の統括、10ms周期制御、制御権調停、ミキシング、フェイルセーフ判定、定期ログ出力 |
| **`MotorController`** | LEDC PWM初期化、モータ真理値表に基づくデューティ出力、停止・ブレーキ制御 |
| **`PS4Input`** | Bluetooth Classic 管理、接続状態監視、スティック値デッドゾーン適用・スケール、ボタン状態取得 |
| **`IRReceiver`** | IRremote による 2ピン同時デコード、NECコード解析、リピート判定、150msタイムアウト・復帰判定 |
| **`Config.h`** | ハードウェアピン定義、PWM周波数・分解能、各種タイムアウト・制御周期定数 |
| **`Types.h`** | 制御ソース (`ControlSource`)、動作コマンド (`MotionCommand`)、入出力データ構造体定義 |

---

## 操作仕様

### 1. PS4コントローラ操作

- **前後移動**: 左スティック (LY)  
  上に倒すと前進、下に倒すと後退。
- **左右旋回**: 右スティック (RX)  
  右に倒すと右旋回、左に倒すと左旋回。
- **同時入力 (ミキシング)**:  
  前後移動しながらの旋回が可能。
  $$\text{leftMotor} = \text{throttle} + \text{steering}$$
  $$\text{rightMotor} = \text{throttle} - \text{steering}$$
  出力が $\pm 255$ を超える場合は比率を維持したまま最大値を 255 にスケーリングします。
- **ショートブレーキ**: **R1 ボタン**  
  押下中は直ちに両輪ショートブレーキ (HH) がかかります。

### 2. 赤外線リモコン操作 (NECプロトコル)

テンキー操作に対応（例として一般的な 32bit NEC コード `0x00FFXXXX` をデコード）。

| キー | コマンド | 動作 |
|:---:|:---:|---|
| **2** | `0x18` | 前進 |
| **4** | `0x10` | 左超信地旋回 |
| **5** | `0x38` | ショートブレーキ |
| **6** | `0x5A` | 右超信地旋回 |
| **8** | `0x4A` | 後退 |

- ボタン長押し中はリピートコードにより動作が継続します。
- 送信を離すと **150ms** でタイムアウトし惰性停止 (LL) します。
- 信号途絶後 **650ms** (150ms + 500ms) 経過すると、PS4コントローラ制御へ復帰します。

---

## シリアルログ仕様 (115200 bps)

### 起動・初期化時
赤外線受信機（IR2, IR4）の初期化完了、ESP32 の Bluetooth MAC アドレス、および PS4 ホスト初期化メッセージが出力されます。
```text
IR Receiver [IR2] initialized on GPIO 2
IR Receiver [IR4] initialized on GPIO 4
System Start
ESP32 Bluetooth MAC:
XX:XX:XX:XX:XX:XX
PS4 Controller Host initialized. Waiting for controller...
```

### コントローラ接続状態遷移
```text
PS4 Connected
PS4 Disconnected
```

### 赤外線受信時
受信した 32bit コードを 16進数（8桁）で表示します。
```text
IR2 : 0x00FF18E7
IR4 : 0x00FF10EF
```

### 制御ソース切り替え時
```text
Control Source : IR2
Control Source : IR4
Control Source : PS4
```

### 1秒周期ログ
IR制御中であっても PS4 入力状態および現在のモータ出力を常時表示します。
```text
PS4:
LY=0
RX=0

Motor:
L=0
R=0
```

---

## 開発環境とビルド手順

### 推奨環境
- Visual Studio Code
- PlatformIO IDE 拡張機能
- ESP32 プラットフォーム (Espressif 32)
- Arduino フレームワーク

### 使用ライブラリ
- **IRremote** (`z3t0/IRremote @ ^4.4.1`)
- **PS4Controller** (`lib/PS4-esp32`, ベース: [404background/PS4-esp32](https://github.com/404background/PS4-esp32))

### PlatformIO 設定 (`platformio.ini`)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    z3t0/IRremote@^4.4.1
```

### ビルド・書き込みコマンド

PlatformIO CLI を使用する場合:

```bash
# ビルド
pio run

# ESP32 への書き込み
pio run -t upload

# シリアルモニタ起動 (ボーレート 115200)
pio device monitor
```

---

## PS4コントローラのペアリング手順

1. 本プログラムを ESP32 に書き込み、シリアルモニタを起動します。
2. 起動時に表示される `ESP32 Bluetooth MAC: XX:XX:XX:XX:XX:XX` をメモします。
3. PC に PS4 コントローラを USB 接続し、ペアリングツール（[SixaxisPairTool](https://dancingbytes.com/sixaxispairer/) 等）を使用して PS4 コントローラの接続先 MAC アドレスをメモしたアドレスに書き換えます。
4. USB ケーブルを外し、PS4 コントローラの **PS ボタン** を押します。
5. ESP32 と接続が確立すると、シリアルモニタに `PS4 Connected` と表示され、操作可能になります。

---

## プロジェクト構成

```text
ARC2026/
├── docs/
│   ├── Architecture.md      # システム設計・アーキテクチャ仕様書
│   └── Requirements.md      # 要件定義書
├── include/
│   ├── Config.h             # ピン配置・制御パラメータ定数
│   ├── Types.h              # 列挙型・データ構造体定義
│   ├── MotorController.h    # モータ制御ヘッダ
│   ├── PS4Input.h           # PS4コントローラ入力ヘッダ
│   ├── IRReceiver.h         # 赤外線受信ヘッダ
│   └── RobotController.h    # ロボット統括制御ヘッダ
├── src/
│   ├── main.cpp             # エントリーポイント (setup / loop)
│   ├── MotorController.cpp  # モータドライバ制御実装
│   ├── PS4Input.cpp         # PS4入力処理実装
│   ├── IRReceiver.cpp       # 赤外線受信・デコード実装
│   └── RobotController.cpp  # 全体制御・優先度調停実装
├── lib/
│   └── PS4-esp32/           # PS4Controller ライブラリ
├── platformio.ini           # PlatformIO プロジェクト設定ファイル
└── README.md                # 本ドキュメント
```
