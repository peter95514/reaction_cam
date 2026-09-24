# ReactionTest 反應力測試

以攝影機 + 骨架偵測（YOLOv8-pose）判斷使用者是否完成指定動作，並測量反應時間的桌面程式。

> 本文件同時是「與 Claude 溝通用的專案說明書」。開新對話時請先貼上 README.md 與 CONVENTIONS.md。
> 標記 `TODO` 的地方代表尚未完成或尚未決定，請隨進度更新。

---

## 1. 目的

- 程式給出提示（例如：「請舉起左手」）後，用攝影機偵測使用者是否做出正確動作。
- 動作是否正確由**關節角度規則**判斷（例如：左肩角 140°~180° 且左肘角 130°~180° 視為舉手）。
- 記錄從提示出現到動作通過判定的時間，作為反應力成績。
- 介面使用 GLFW + Dear ImGui，語系為繁體中文。

## 2. 目前進度

| 模組 | 狀態 | 說明 |
|---|---|---|
| `MotionDetector` | 可用 | YOLOv8n 找人 (ROI) + YOLOv8n-pose 取 17 個關鍵點 |
| `ActionEvaluator` | 可用 | 依 `AngleRule` 計算角度、加權評分、判定通過 |
| `GuiController` | 可用 | 視窗、ImGui 初始化、畫面切換主迴圈 |
| `MenuScreen` | 可用 | 主選單（開始測試 / 設定 / 離開） |
| `SettingScreen` | TODO | 尚未建立 |
| `TrialScreen` / `TrialController` | TODO | `TrialController.cpp` 目前是空檔 |
| 攝影機畫面整合進 ImGui | TODO | 目前只有 `src/main.txt` 的 OpenCV 視窗原型 |
| 成績記錄 / 存檔 | TODO | `.gitignore` 已預留 `*.csv`、`data/` |

`src/main.txt` 是早期的 OpenCV 視窗原型（左手舉起偵測），副檔名為 txt 所以不會被編譯，僅作為整合時的參考。

## 3. 架構

```
main.cpp
  └─ GuiController          視窗 + 主迴圈 + 畫面切換
       └─ Screen (介面)      enter() / tick() / exit()
            ├─ MenuScreen    已完成
            ├─ SettingScreen TODO
            └─ TrialScreen   TODO  ← 會使用 TrialController
                   │
                   ├─ MotionDetector   攝影機影格 → ROI → 關鍵點
                   └─ ActionEvaluator  關鍵點 → 角度 → 分數 / 是否通過
```

### 畫面切換機制
- 每個 `Screen` 的 `tick()` 每幀被呼叫一次，回傳「下一個要顯示的 `ScreenId`」。
- 回傳值與目前畫面不同時，`GuiController` 會呼叫 `exit()` → 切換 → `enter()`。
- 回傳 `ScreenId::Exit` 結束程式。
- 新增畫面步驟：繼承 `Screen` → 在 `ScreenId` 加項目（如需要）→ 在 `main.cpp` 用 `registerScreen()` 註冊。

### 資料流（單一影格）
1. `MotionDetector::get_rois(frame)`：YOLOv8n 找出人物框（單人場景只取第一個）。
2. `MotionDetector::detect_skeleton(roi)`：YOLOv8n-pose 輸出 17 個 COCO 關鍵點。
3. `ActionEvaluator::evaluate(keypoints)`：套用角度規則，回傳 `EvalResult`。
4. 評估使用 ROI 座標系（角度不受平移影響）；只有繪圖時才需要加回 ROI 偏移量。

### EvalResult 語意
- `valid == false`：可見關鍵點太少，**這一幀不該拿來判斷**（不等於動作錯誤）。
- `valid == true`：`score` 為 0~100，`passed = score >= pass_score`。

### 定義動作規則範例
```cpp
ActionEvaluator evaluator(0.4f, 80.f, 0.5f);  // 最低信心, 通過分數, 最低有效權重比例
evaluator.changerule({
    {"L_shoulder", L_HIP,      L_SHOULDER, L_ELBOW, 140.f, 180.f, 2.0f},
    {"L_elbow",    L_SHOULDER, L_ELBOW,    L_WRIST, 130.f, 180.f, 1.0f},
});
```
`AngleRule` 欄位：`{名稱, 點A, 頂點B, 點C, 最小角度, 最大角度, 權重}`，角度為 ∠ABC。

## 4. 環境需求

- C++17 編譯器、CMake ≥ 3.14
- OpenCV（含 `dnn` 模組），需先自行安裝並讓 CMake 找得到
- OpenGL
- GLFW 3.4、Dear ImGui v1.91.9：由 CMake `FetchContent` 自動下載（第一次 build 需要網路）
- 模型檔：`yolov8n.onnx`、`yolov8n-pose.onnx`（輸入 640×640）
  - 可用 ultralytics 匯出：`yolo export model=yolov8n.pt format=onnx imgsz=640`（pose 版同理）
- 字型：`fonts/NotoSansTC-Regular.ttf`（找不到時中文會顯示成 `?`）

## 5. 建置與執行

```bash
cmake -B build
cmake --build build
./build/ReactionTest        # Windows 依產生器不同，可能在 build/Debug/ 底下
```

CMake 會把專案根目錄的 `model/`、`fonts/` 在 build 後複製到執行檔旁邊（目錄不存在就跳過）。

**注意：** 目前 `MotionDetector` 是用 `"yolov8n-pose.onnx"` 這個相對路徑載入，也就是從「執行時的工作目錄」找，
**不是** `model/` 底下。詳見「已知問題」。

## 6. 目錄結構

```
.
├─ CMakeLists.txt
├─ README.md
├─ CONVENTIONS.md      編寫習慣（與 Claude 溝通用）
├─ include/            標頭檔
├─ src/                實作檔（CMake 以 GLOB_RECURSE 自動收集 src/*.cpp）
├─ model/              ONNX 模型（不進版控與否請自行決定）
└─ fonts/              字型
```
新增 `.cpp` 後不用改 CMakeLists，但需要重新執行 cmake（已設定 `CONFIGURE_DEPENDS`，一般 build 也會自動偵測）。

## 7. 已知問題 / 待辦

依我閱讀程式碼發現的項目，優先度由高到低：

1. **`Screen` 基底類別沒有實作也不是純虛擬函式**：`Screen.h` 宣告了 `~Screen()`、`enter()`、`exit()`、`tick()` 卻沒有定義，
   會造成連結錯誤（undefined reference to vtable）。建議改成 `virtual ~Screen() = default;` 與 `virtual ScreenId tick() = 0;` 等。
2. **`ScreenId::Setting`、`ScreenId::Trial` 尚未註冊**：在選單按「開始測試 / 設定」時，`screenid_.at()` 會丟出 `std::out_of_range`。
   在畫面完成前可先註冊空畫面或暫時擋掉。
3. **模型路徑不一致**：CMake 複製到 `<exe>/model/`，程式卻讀 `"yolov8n-pose.onnx"`。建議統一為 `model/yolov8n-pose.onnx`，
   並在模型載入失敗時給出明確錯誤（目前 `readNetFromONNX` 失敗才會在 forward 時出錯）。
4. **`MotionDetector` 未使用/未定義成員**：`bg_subtractor`、`min_area` 以外的 MOG2 相關程式已不使用；
   `get_skeleton()` 只有宣告沒有定義；`is_tracking_skeleton_or_not` 沒有初始化。
5. **`ActionEvaluator::EvalResult::feedback`** 目前永遠是空的，尚未產生「哪個角度不對」的提示文字。
6. **`detect_skeleton` 只取信心最高的一個人**，且 `confThreshold = 0` 等於不過濾個別關鍵點（過濾交給 `ActionEvaluator` 的 `min_conf_`）。
   若日後要多人場景需重構。
7. `MotionDetector::get_rois` 每次呼叫都重新配置 padded / blob，效能有空間；YOLO 推論建議不要在 UI 執行緒每幀跑
   （原型用「每 5 幀偵測一次」緩解），之後整合 ImGui 時可考慮獨立執行緒。

## 8. 預計流程（測試畫面設計草稿）TODO

1. 進入 Trial → 開啟攝影機、顯示「準備」。
2. 隨機延遲後顯示提示動作，開始計時。
3. 連續 N 幀 `passed == true` 視為完成，停止計時。
4. 顯示成績；可重測或回選單。
5. （選用）將成績寫入 `data/` 底下的 CSV。

> 以上為初步構想，實際規則（連續幀數、隨機延遲範圍、動作清單）請在此更新。

## 9. 與 Claude 合作的方式

- 請求修改時，請附上相關檔案的最新內容，避免我依過期程式碼修改。
- 我預設會遵守 `CONVENTIONS.md`；如有衝突以 `CONVENTIONS.md` 為準。
- 一次只處理一個功能；改動會標明「新增 / 修改 / 刪除了哪些檔案」。
- 我看不到你的執行結果，回報問題時請貼完整的編譯錯誤或終端輸出。
