# Humidifier controller

ESP32 Arduino core 2.0.17 用の子機です。ESP-NOW のチャンネル1で親機から操作します。

1. `token.h.example` を `token.h` にコピーし、`HUMIDIFIER_PARENT_MAC_ADDR` に親機の **STA MAC** を設定します。
2. 子機を起動し、115200 baud のシリアル出力で `Humidifier STA MAC` を確認します。
3. 親機の `main/room_environment_manager/token.h` に子機の **STA MAC** を追加します。

   ```cpp
   #define HUMIDIFIER_MAC_ADDR {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
   ```

親機にこの定義がなければ加湿器との通信は無効です。定義を残して加湿器を取り外しても、親機は通常動作を続けます。設定変更後はそれぞれのスケッチを書き込み直してください。

| GPIO | 接続 |
| --- | --- |
| 17 | 給水LED（LOWで点灯） |
| 18 | 弱運転LED（LOWで点灯） |
| 23 | 強運転LED（LOWで点灯） |
| 4 | 操作ボタンのドライバー（OFF → WEAK → STRONG → OFF） |

ボタン出力は既定で HIGH が押下、LOW が解放です。回路に合わせて `HUMIDIFIER_BUTTON_ACTIVE_LEVEL` を変更してください。200ms押下後に250ms待ち、80ms安定したLEDの状態で判定します。必要に応じて `HM_BUTTON_MS` / `HM_SETTLE_MS` を実機に合わせて調整してください。親機の応答待ちは既存の1500ms枠なので、押下時間を長くする場合はそちらも確認します。

操作は非同期に実行し、最大3回の押下で打ち切ります。給水LED点灯中は運転開始を拒否し、弱・強が同時点灯した場合は状態不明として操作を中止します。親機は実際のLED状態を伴う完了応答で成功を確認します。親機との通信が5分間途絶えた場合、子機はリモート運転を停止しようとします（同じく最大3回）。

`humidifier_protocol.h` は親機の同名ファイルと同一に保ちます。独立したArduinoスケッチとしてビルドできるよう、両ディレクトリに配置しています。
