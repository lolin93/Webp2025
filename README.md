# Webp2025｜網頁設計與開發

本儲存庫收錄 2025 年網頁設計與開發課程的課堂練習與作業，內容涵蓋 Python／Django 後端開發、API 操作、HTML／CSS 網頁設計、JavaScript 互動程式，以及 React 前端元件開發。

透過選課表、個人介紹網站、打字互動、展覽資訊查詢與登入介面等練習，逐步學習網頁從內容呈現、使用者互動到外部資料串接的開發流程。

## 課程資訊

| 項目 | 內容 |
| --- | --- |
| 授課教師 | 吳齊人 |
| 上課方式 | 實體上課 |
| 課程期間 | 2025/02/19－2025/06/04 |
| 儲存庫 | [lolin93/Webp2025](https://github.com/lolin93/Webp2025) |

## 技術與工具

| 技術 | 應用內容 |
| --- | --- |
| HTML、CSS | 網頁結構、表格、版面與視覺樣式 |
| Bootstrap | 按鈕、表格、格線與個人網站排版 |
| JavaScript | 事件處理、字串操作、亂數與 DOM 更新 |
| XMLHttpRequest、Fetch API | 取得外部資料與處理 JSON |
| Python、Django | 後端應用程式與資料模型 |
| Django REST Framework | API 請求處理與資料回應 |
| React | 元件化介面、事件處理與狀態管理 |
| Material UI、MUI DataGrid | 登入表單、圖示按鈕與資料表格 |

## 課堂練習

| 目錄 | 主題 | 實作內容 |
| --- | --- | --- |
| [Lab#1](./Lab%231/) | Django API 入門 | 讀取 `name` 查詢參數，回傳問候訊息與 HTTP 狀態碼 |
| [Lab#2](./Lab%232/) | Django 資料操作 | 建立文章資料，並以 JSON 回傳文章列表 |
| [Lab#3](./Lab%233/) | HTML 表格與 CSS | 製作資工選課表，設定框線、表頭與交錯背景色 |
| [Lab#4](./Lab%234/) | DOM 操作 | 透過按鈕修改選課表中的教師姓名 |
| [Lab#5](./Lab%235/) | JavaScript 基礎 | 使用函式、迴圈與字串重複，在主控台輸出聖誕樹 |
| [Lab#6](./Lab%236/) | 動態元素操作 | 新增與刪除按鈕，搭配流水編號與 Bootstrap 樣式 |
| [Lab#7](./Lab%237/) | 鍵盤事件 | 比對輸入字元、移除正確字元並產生亂數字串 |
| [Lab#8](./Lab%238/) | 進階打字互動 | 累計連續錯誤次數，達到條件時額外加入亂數字元 |
| [Lab#9](./Lab%239/) | 展覽資料串接 | 讀取文化部展覽資料，呈現名稱、地點與票價，並提供清除功能 |
| [Lab#10](./Lab%2310/) | Flickr 圖片牆 | 使用 Fetch API 取得圖片資料並動態建立圖片元素 |
| [Lab#11](./Lab%2311/) | React 入門 | 顯示 Hello CGU 文字，練習行內樣式與點擊事件 |
| [Lab#12](./Lab%2312/) | React 元件 | 組合文字元件與 Material UI 圖示按鈕 |
| [Lab#13](./Lab%2313/) | 登入介面設計 | 使用 Material UI 建立帳號、密碼與記住我等表單介面 |
| [Lab#14](./Lab%2314/) | React 介面練習 | `react-game` 專案目前的入口程式呈現登入元件 |

## 作業內容

### HW#1｜課程資料 API

使用 Django 與 Django REST Framework 建立課程資料操作功能。

- 儲存開課單位、課程名稱與授課教師。
- 提供新增課程與查詢課程列表的功能。
- 以 JSON 格式回傳資料。

📁 [查看程式碼](./HW%231/hello_django/)

### HW02｜個人介紹網站

使用 HTML、CSS 與 Bootstrap 製作個人介紹頁面，練習資訊組織與主內容、側邊欄的版面配置。

- 個人簡介與學經歷。
- 競賽與得獎紀錄。
- 個人照片與聯絡資訊。
- 頁首、頁尾與返回頁面頂端連結。

📁 [查看程式碼](./hw02/)

### HW#3｜展覽資訊搜尋與分頁

串接文化部展覽資料，將取得的 JSON 資料轉換成可查詢的表格。

- 顯示展覽名稱、地點與票價。
- 依展覽名稱關鍵字篩選資料。
- 每頁顯示 10 筆資料。
- 提供上一頁、下一頁與頁碼資訊。

📁 [查看程式碼](./HW%233/)

### HW#4｜React 展覽資料表

使用 React 與 MUI DataGrid 建立展覽資訊查詢介面。

- 透過 `useEffect` 在元件載入時取得資料。
- 使用 `useState` 管理展覽資料與搜尋關鍵字。
- 依輸入內容篩選展覽名稱。
- 使用 DataGrid 呈現資料與分頁。

📁 [查看程式碼](./HW%234/react_datagrid/)

## 課程進度

| 週次 | 日期 | 授課主題 |
| :---: | :---: | --- |
| 1 | 2025/02/19 | 課程介紹與 WWW 的歷史及基本概念 |
| 2 | 2025/02/26 | 後端框架與架構：Django 3.0／Python |
| 3 | 2025/03/05 | 後端框架與架構：Django 3.0／Python |
| 4 | 2025/03/12 | 後端框架與架構：Django 3.0／Python |
| 5 | 2025/03/19 | 後端框架與架構：Django 3.0／Python |
| 6 | 2025/03/26 | RESTful API 設計 |
| 7 | 2025/04/02 | RESTful API 設計 |
| 8 | 2025/04/09 | JavaScript 網頁程式設計：HTML、CSS、DOM |
| 9 | 2025/04/16 | JavaScript 網頁程式設計：HTML、CSS、DOM |
| 10 | 2025/04/23 | 前端框架：React |
| 11 | 2025/04/30 | 前端框架：React |
| 12 | 2025/05/07 | 前端框架：React |
| 13 | 2025/05/14 | 前端框架：React |
| 14 | 2025/05/21 | 前端框架：React |
| 15 | 2025/05/28 | 期末專題展示 |
| 16 | 2025/06/04 | 期末專題展示 |

> 上表為課程大綱；Lab 編號為練習編號，不代表對應週次。Django 3.0 為課程大綱列出的版本，實際執行環境需依各專案設定確認。

## 執行方式

各資料夾為獨立練習，請依專案類型開啟。

### HTML／JavaScript 練習

適用於 `Lab#3` 至 `Lab#10`、`hw02` 與 `HW#3`。

取得需要的練習資料夾後，可使用 VS Code Live Server 開啟其中的 `index.html`。若資料夾包含 CSS、JavaScript 或圖片，需一併保留，才能完整呈現頁面。

`Lab#5` 的輸出位於瀏覽器開發者工具的 Console。

### React 專案

進入包含 `package.json` 的專案資料夾，安裝依賴後啟動。以 `HW#4` 為例：

```bash
cd "HW#4/react_datagrid"
npm install
npm start
```

其他 React 專案位置：

- `Lab#11`
- `Lab#12/hello-world`
- `Lab#13/react_login`
- `Lab#14/react-game`

實際使用的依賴版本與可執行指令，以各資料夾的 `package.json` 為準。

### Django 專案

Django 練習位於以下資料夾：

- `Lab#1/hello_django`
- `Lab#2/hello_django`
- `HW#1/hello_django`

準備好相容的 Python、Django 與 Django REST Framework 環境後，在包含 `manage.py` 的資料夾中執行：

```bash
python manage.py migrate
python manage.py runserver
```

API 路徑請參考各專案的 `urls.py` 設定。

## 補充說明

- 本 README 聚焦於儲存庫中的網頁設計與開發練習。
- 登入頁面屬於介面與表單處理練習，目前讀取到的程式未串接後端身分驗證。
- 展覽資訊與 Flickr 圖片牆依賴外部服務，執行結果會受到網路、API 可用性與金鑰設定影響。
- 本儲存庫保留課程練習內容，各子專案的執行環境與依賴可能不同。
