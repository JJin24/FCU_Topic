# USTC-II 相關程式內容

目前使用 `make` 指令將會編譯出 4 個程式，分別是 Flow2img、Flow2img_II、Capture_Flow、Flow2img_II。相關程式碼的功能及用途如下。

## 使用的第三方套件與授權

本專案使用了一些開源的第三方套件，詳細的授權資訊請參考 [NOTICE](../NOTICE.md) 檔案。

- **HAST_IDS**: 主要概念啟發自 [echowei/DeepTraffic](https://github.com/echowei/DeepTraffic) 專案 (MPL-2.0 License)。
- **uthash.h**: 使用來自 [troydhanson/uthash](https://github.com/troydhanson/uthash) 的 `uthash.h`。
- **stb_img_write.h**: 使用來自 [nothings/stb](https://github.com/nothings/stb) 的 `stb_image_write.h`。

## 編譯

1. 請先安裝以下套件 `libpcap-dev`、`redis-server`、`redis-tools`

```bash
apt install libpcap-dev redis-server redis-tools
```

2. 在此資料夾中使用 `make` 指令將會自動編譯此上述的 4 個程式。

```bash
make
```

## [Flow2img](./Flow2img.c)

### 說明

相關處理方式請參考 [HAST-IDS: Learning Hierarchical Spatial-Temporal Features Using Deep Neural Networks to Improve Intrusion Detection](https://ieeexplore.ieee.org/document/8171733) 這篇論文。

對於 [Flow2img](./Flow2img.c) 程式來說，他將支援兩種模式的圖片轉換，`HAST_ONE` 及 `HAST_TWO` 兩種模式。其同時也是 `Flow2img_II` 及 `HAST_Two`  兩個程式的原型。

- `HAST_ONE`：從一個 pcap 檔案中提取前 n 位元組出來，轉換成一個 one hot encoded 編碼的圖片。
- `HAST_TWO`：從一個 pcap 檔案中，提取前 n 的封包，再從每一個封包中提取前 m 位元組出來，轉換成一個 one hot encoded 的圖片。

> [!NOTE]
> 可拓展選項 [de_addr_noise](./de_addr_noise.c)
> 
> 目前程式可以透過在最尾端加入 de_addr_noise 的參數，將 MAC Address 及 IP Address 的部分在進行 one hot encoded 的時候可以不將該位置的位元組進行編碼的動作。

### 使用方式

1. 顯示說明

```bash
./Flow2img -h
```

2. 使用 `HAST_ONE` 模式，將存有 pcap 的資料夾 `<pcap_folder>` 中的所有 `.pcap` 檔案提取前 `<m_bytes>` 位元組後，將輸出的圖片存放在 `<output_img_folder>` 中。(可選用 `<opt>` 將第一個封包的 MAC Address 及 IP Address 去除)

```bash
./Flow2img -1 <pcap_folder> <output_img_folder> <m_bytes> <opt>
```

3. 使用 `HAST_TWO` 模式，將存有 pcap 的資料夾 `<pcap_folder>` 中的所有 `.pcap` 檔案先提取前 `<n_packets>` 個封包後，並且再從每一個封包中提取前 `<m_bytes>` 後做 one hot encoded 轉換成圖片 (對於未滿足 `<n_packets>` 及 `<m_bytes>` 條件的封包將會自動填補空圖片或是空像素)。將輸出的圖片存入 `<output_img_folder>` 資料夾中。(可選用 `<opt>` 將封包的 MAC Address 及 IP Address 去除)

```bash
./Flow2img -2 <pcap_folder> <output_img_folder> <n_packets> <m_bytes> <opt>
```

> [!NOTE]
> 拓展使用
> 
> [`Convert_All_Flow.sh`](./Convert_All_Flow.sh) 及 [`Convert_All_Flow_Ver_Colab.sh`](./Convert_All_Flow_Ver_Colab.sh) 兩者皆將可以轉換存放 pcap 資料夾的資料夾中所有子資料夾自動帶入到 `Flow2img` 程式中進行圖片的轉換。
> 
> [`Convert_All_Flow_Ver_Colab.sh`](./Convert_All_Flow_Ver_Colab.sh) 支援輸入不同參數，而 [`Convert_All_Flow.sh`](./Convert_All_Flow.sh) 已經將參數寫在程式裡面，使用方式如同 **使用方式** 所示。

## [Flow2img_II](./Flow2img_II.c) (不建議使用)

### 說明

對於 [Flow2img_II](./Flow2img_II.c) 程式來說，他使用 [Flow2img](./Flow2img.c) 作為程式的拓展。其加上從 UNIX Domain socket (位在 `/tmp/Flow2img_Proxy.sock`) 讀取 `capture_Flow` 傳送的封包，並將 socket 設定為 non-blocking I/O 並使用 epoll 處理資料的傳入。接著建立一次性 Thread 的 HAST_TWO 程式做處理，最終將圖片、pcap、pcap 資訊記錄成同一筆 HSET 後傳送到 Redis 資料庫中。

### 使用方式

```bash
./Flow2img_II
```

### 已知問題

- 從 socket 讀取資料只會讀取一次
- 頻繁的建立 Thread 將會使系統效能變慢

## [HAST_Two](./HAST_Two.c)

### 說明

此程式基於 [`Flow2img_II`](./Flow2img_II.c) 為原型，對程式加上 Thread Pool、socket 讀取方式調整功能及將程式分散為 [`common.h`](./common.h)、[`main_server.c`](./main_server.c)、[`HAST_Two.c`](./HAST_Two.c)、[`redis_handler.c`](./redis_handler.c)、[`thread_pool.c`](./thread_pool.c) 的檔案中，方便後續維護程式使用。

Thread Pool 的相關實現可參考 [`thread_pool.c`](./thread_pool.c) 及 [`thread_pool.h`](./thread_pool.h) 中的註解。

socket 將會先讀一個由 client 發出的 **長度封包**，接著再讀取 **內容封包**。得益於 EPOLLET 及 non blocking I/O 的設定，使得可以讀取可以先讀取封包，再去判斷是何種類型的封包做對應的處理。

socket 傳送的資料格式如下 (待補充)

### 使用方式

```bash
./HAST_Two
```

### 傳送端傳送 Flow2img_II_context 方式

傳送端傳送一個 pcap 資料需要先後傳送兩個 socket 到 `HAST_Two` 所設定的 socket (`/tmp/Flow2img_II_Proxy.sock`)。

1. 紀錄 `Flow2img_II_Proxy` 結構大小的封包 (`sizeof(Flow2img_II_Proxy)`)。
2. 傳送已經建立好的 `Flow2img_II_Proxy` 結構。

> [!WARNING]
>
> 在傳送 `Flow2img_II_Proxy` 結構前需要將內部非 `uint8_t` 和 `char` 類型及其陣列類型外的資料轉換成大端序 (使用 `htonl` 或 `htons` 轉換)。

> [!NOTE]
>
> 為什麼需要先傳送 `Flow2img_II_Proxy` 結構大小的封包?
>
> 因為當初設計 `Flow2img_II_Proxy` 結構的時候，因為不確定傳送端會傳送多大的 pcap 資料過來，所以設計先讀取 `pcap_size` 後再替 `pcap_data` 準備 buffer 使用。後期因為直接先定義 `pcap_data` 可用的 buffer 大小為 `sizeof(uint8_t) * MAX_PKT_LEN * N_PKTS` 個 Bytes，所以可以直接實際上該步驟並沒有實際的幫助。
>
> 但為使其他尚未記載的資訊可以方便先行傳送，因此目前暫時保留此傳送機制。