# 編寫習慣 (CONVENTIONS)

這份文件描述本專案的程式風格，也是與 Claude 溝通時的規則。
內容是依現有程式碼整理出來的，標記 **（待確認）** 的項目是我推測的，請你確認或修改。

---

## 1. 語言與工具

- C++17（CMake 設定 `CMAKE_CXX_STANDARD 17`）
- 格式化：建議使用 `clang-format`，基底風格 Google，縮排 4 空格（**待確認**，若同意可補一份 `.clang-format`）
- 註解與 UI 文字使用**繁體中文**；識別字（變數、函式、類別）使用英文

## 2. 命名

| 類型 | 規則 | 範例 |
|---|---|---|
| 類別 / struct / enum | PascalCase | `ActionEvaluator`、`AngleRule`、`ScreenId` |
| enum class 值 | PascalCase | `ScreenId::Menu` |
| 舊式 enum 值（關節） | 全大寫 SNAKE | `L_SHOULDER` |
| 新增的函式 | camelCase | `computeAngle`、`registerScreen`、`switchTo` |
| 私有成員變數 | 小寫 + 結尾底線 | `rules_`、`min_conf_`、`window_` |
| 區域變數 | camelCase 或短小寫 | `safeRoi`、`bestconf` |
| 常數 | 全大寫 SNAKE | `DETECT_INTERVAL`、`PERSON_CLASS_ID` |
| 命名空間 | 小寫 | `ui` |
| 檔名 | 與類別同名 | `MenuScreen.h` / `MenuScreen.cpp` |

**歷史包袱（新程式碼請勿沿用）：**
- `MotionDetector` 使用 snake_case 函式（`get_rois`、`detect_skeleton`）與無底線成員（`min_area`）。
- `ActionEvaluator::changerule` 全小寫。
- 若要統一，請一次性改名並單獨作為一個 commit，不要與功能修改混在一起。**（待確認：是否要統一？）**

## 3. 檔案組織

- 標頭放 `include/`，實作放 `src/`，一個類別一組 `.h/.cpp`。
- 標頭一律 `#pragma once`，不使用 include guard 巨集。
- 小型、無狀態的 UI 輔助函式可以 header-only，放在 `namespace ui` 並標 `inline`（如 `Ui.h`、`Theme.h`）。
- `.cpp` 中先 include 自己的標頭，再 include 第三方，再 include 標準庫；專案標頭用 `"..."`，第三方與標準庫用 `<...>`（**待確認**，目前專案內混用，新程式碼請依此規則）。
- 標頭中不要放 `using namespace`。

## 4. 類別設計

- 成員變數寫在 `private:`，公開函式寫在 `public:`；標頭排列順序：private 成員 → public 介面。
- 建構子使用成員初始化列表。
- 覆寫虛擬函式一律加 `override`。
- 有虛擬函式的基底類別，解構子要是 `virtual`。
- 畫面（Screen）的規則：
  - `tick()` 每幀執行，**不可阻塞**、不可做耗時運算（例如同步跑模型推論）。
  - `tick()` 只回傳下一個 `ScreenId`，不直接操作 `GuiController`。
  - 資源的取得與釋放放在 `enter()` / `exit()`。

## 5. 錯誤處理

- 初始化階段（視窗、模型、字型）失敗：丟出 `std::runtime_error`，訊息要說明是哪個資源。
- 每幀執行路徑：不丟例外；以回傳值表達失敗（例如 `computeAngle` 回傳 `bool`）。
- 「資料不足，這一幀不算數」與「動作不合格」要分開表示（參考 `EvalResult::valid` 與 `passed`）。
- 不使用 `std::cout` 除錯後遺留在程式中；暫時性的除錯輸出請標 `// DEBUG` 方便搜尋刪除。

## 6. UI（ImGui）

- 每個畫面使用 `ui::beginFullscreen()` 開頭，最後一定要呼叫 `ImGui::End()`。
- 版面用 `ui::centeredText`、`ui::centeredButton` 等共用輔助函式，新增輔助函式放進 `Ui.h`。
- 顏色、字型、間距統一在 `Theme.h` 設定，畫面程式碼中不要寫死顏色（狀態提示色除外）。
- 按鈕文字用繁體中文；所有含中文的字串仍需確保檔案為 UTF-8。

## 7. 座標與單位

- 關鍵點座標：ROI 座標系用於評估，畫面座標系（加回 ROI 偏移）只用於繪圖。
- 角度單位：**度**（degree），範圍 0~180。
- 時間單位：毫秒（ms），使用 `std::chrono::steady_clock`。**（待確認）**
- 信心值 / 分數：信心 0~1，分數 0~100。

## 8. 註解

- 只寫「為什麼」，不重述程式碼在做什麼。
- 需要提醒的地方用 `TODO:` / `FIXME:` 前綴。
- 公開介面的重要語意寫在標頭的欄位或函式旁（例如 `EvalResult::valid` 的註解）。

## 9. Git

- `.gitignore` 已排除：`build/`、`*.csv`、`*.db`、`*.log`、`data/`、IDE 設定等。
- 模型檔（`*.onnx`）目前**沒有**被忽略；檔案較大，請決定要進版控、用 Git LFS，或加入 `.gitignore` 並在 README 說明取得方式。**（待確認）**
- Commit 訊息用繁體中文或英文皆可，一個 commit 只做一件事。**（待確認）**

## 10. 與 Claude 溝通時的規則

請 Claude 在修改或新增程式碼時遵守：

1. **先遵守本文件**；如果現有檔案風格與本文件衝突，新程式碼依本文件，並在回覆中指出衝突處而不是默默改掉舊程式碼。
2. **最小改動**：只改與需求相關的部分，不順手重構、不重新排版整個檔案。
3. **完整檔案 vs. 片段**：小改動給片段並標明位置；超過約 30 行或跨多處修改時，給完整檔案。
4. 新增檔案時要說明放在哪個目錄，以及是否需要改 `CMakeLists.txt`。
5. 回覆使用繁體中文；程式碼中的識別字用英文，註解用繁體中文。
6. 不確定時（例如動作判定規則、成績計算方式）先問，不要自行假設。
7. 發現潛在問題（如未初始化變數、執行緒安全、每幀配置記憶體）時，要提出，但另列為建議，不與本次修改混在一起。
8. 不引入新的第三方函式庫，除非先徵得同意；若需要，說明原因與 `FetchContent` 寫法。

## 11. 待你決定的事項（勾選後可刪除此區）

- [ ] 是否統一 `MotionDetector` / `changerule` 的命名風格
- [ ] 是否加入 `.clang-format`
- [ ] 模型檔是否進版控
- [ ] Include 順序與引號規則是否採用上述建議
- [ ] 計時單位與計時器選擇
