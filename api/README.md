# API 使用教學

## 前置作業

- 在部署此 API 前，需要先在系統中安裝 `nodejs` v.22.19.0 (LTS) 或更高版本及 `npm`。可以參考 `nodejs` [官方教學](https://nodejs.org/en/download) 來安裝。

1. 使用終端機到此目錄 (`/api/`) 中使用 `npm` 安裝依賴套件

    ```bash
    npm install
    ```

2. 至 [FireBase Console](https://console.firebase.google.com/) 中下載 Firebase Admin SDK 金鑰。

    [FireBase Console](https://console.firebase.google.com/) &rarr; 對應專案 &rarr; 左側工具列 &rarr; 專案設定 &rarr; 服務帳戶 &rarr; Firebase Admin SDK &rarr; Node.js &rarr; 產生新的私密金鑰。

3. 將下載的 Firebase Admin SDK 金鑰移動到 `/api/`，並重新命名為 `FCM_Key.json` 以方便 API 使用 FCM 的通知發送。
4. 編輯 [.env](./.env) 檔案中所有的設定，此配置檔案將會被運用在資料庫連線的設定上面。
5. 現在 API 使用端口 3000，使用前請將在防火牆中做對應的設定，避免無法連線。
6. 目前 API 僅使用 HTTP 的連線，若要加上 HTTPS，請自行設定。

## 使用教學

使用終端機移動到 `/api/` 後，使用以下指令執行即可對 `http://<server-ip>:3000` 使用 API。

```bash
node index.js
```

目前每一個 API 皆可透過 `http://<server-ip>:3000/api-docs` 查詢使用的方法並測試執行。

## 程式說明

API 的所有程式是基於 `nodejs` 的 `express` 套件所完成的，並且透過 `swagger` 完成 API 文件的撰寫。 請在編寫 API 接口時一併完成 API 文件，方便後續維護使用。

- [/api/services/](./services) 資料夾：此資料夾主要為各 API 對資料庫的處理程式。另外含有 `notification.js` 處理通知發送、`datsbase.js` 處理資料庫連線暨相關設定。
- [/api/controllers/](./controllers) 資料夾：此資料夾主要為前端與後端的交互層，處理數據的轉換及回應對應的 HTTP 狀態碼。
- [/api/routes/](./routes) 資料夾：此資料夾建立所有前端路徑的處理，若需要建立新的 API，請從此頁面開始，並將 API 的使用方式也在此頁。
- [/api/index.js](./index.js) 定義所有根路由的處理頁面，如須增加新的根路由，請從此頁開始。

> [!NOTE] `swagger` 及 `router.get` 路徑差異
> 
> 在撰寫 swagger 的說明文件時，路徑須包含根目錄的路徑。例如：在有一個新的路由 /alert/status/notDeal，
> 需要定義在 [/api/routes/alertRoutes.js](./routes/alertRoutes.js)，`router.get` 需要使用 `/status/notDeal` 定義路由，
> 但在 `swagger` 中則需要完整的路徑 `/alert/status/notDeal`。