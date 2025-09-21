# Predit 程式說明

## 前置處理

在使用 `predit.py` 前需要建立一個 python 的虛擬環境，並且使用 `pip` 安裝以下套件

```bash
pip install -r requirements.txt
```

套件安裝完成後，請將 [config.py](./config.py) 中的連線資訊，模型設定設定完成，供後續程式使用。

## 檔案說明

- [Predit.py](./Predit.py)：在實際辨識時的主程式，其依賴 [mariadb_tool.py](./mariadb_tool.py), [model_architectures.py](./model_architectures.py),
[model_tool.py](./model_tool.py), [predit_api.py](predit_api.py), [redis_tool.py](./redis_tool.py) 這五個程式。

    在模型辨識的時候，`Predit.py` 會載入模型，依序建立與 Redis 及 MariaDB 資料庫的連線後，不斷去 Redis 資料庫中找流量來辨識。
    當辨識結果為正常流量的時候，模型會僅將 5 元組等基本資訊儲存到 MariaDB 中。發生異常流量的時候，除了基本資訊以外，同時也將會上傳 
    PCAP、辨識結果等等至 MariaDB 中，並透過冷卻時間來判斷是否將此比惡意流量透過 API 來發送通知給應用程式。
- 其餘檔案 `./predit_*.py` 為實驗用來，主要為測試模型的預測結果使用。
    
> [!WARNING]
> 其餘檔案 `./predit_*.py` 注意事項
> 
> 其餘檔案 `./predit_*.py` 由於會需要大量的修改，因此若要查看最新的檔案或是測試結果，請至 [OneDrive](https://drive.jjin24.com) 中資料夾查看模型、訓練程式及所對應的結果。
> 相關檔案將會在專題結束後最統一的整理，在整理前，請忽略這些檔案的存在及更新。
