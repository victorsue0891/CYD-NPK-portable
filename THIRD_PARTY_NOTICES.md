# 第三方素材與依賴

## 字型

`include/ChineseFont.h` 的點陣字形取自 Noto Sans CJK TC Regular，由 `tools/generate_font.py` 產生子集。

- Copyright © 2014–2021 Adobe (http://www.adobe.com/).
- 授權：SIL Open Font License 1.1，全文見 [assets/fonts/OFL.txt](assets/fonts/OFL.txt)。
- 來源與生成方式見 [assets/fonts/NOTICE.txt](assets/fonts/NOTICE.txt)。
- 完整 OTF 檔由工具下載，不納入本 repository；一般韌體編譯只使用已生成的子集。

## 感測器文件

`NPK-sensor-datasheet/土壤氮磷鉀感測器.pdf` 為專案所使用的原始供應商文件。文件內標示著作權屬威海晶合數字礦山技術有限公司；本專案不主張該文件的著作權，也不變更其原有權利條款。規格書勘誤與實作採用的通訊格式見 [docs/DESIGN.md](docs/DESIGN.md)。

## 韌體依賴

依賴由 PlatformIO 下載，版本及來源固定於 [platformio.ini](platformio.ini)，不將其完整原始碼或編譯產物納入 repository。

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)：TFT 顯示驅動。
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)：觸控驅動。
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32)：ESP32 Arduino framework。

上述依賴各依其隨附授權使用，這份註記不取代或修改上游授權。

## 參考專案

CYD 的顯示硬體設定參考 [victorsue0891/CYD-Cyberpunk-Display](https://github.com/victorsue0891/CYD-Cyberpunk-Display)。本專案的 NPK 採集、顯示流程、接線設計與測試另行建立。
