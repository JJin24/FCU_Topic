# Prompt

## Autoencoder_PyTorch

請你幫我生成一個 PyTorch 的程式 (建議可以用 Notebook 的形式儲存) 我想要訓練一個 autoencoder 的模型。

你需要做的動作是：

- 每行註解
- 減少記憶體的佔用 (有選項可以調整)
- 盡可能的使用 GPU (請毫不客氣的壓榨它)
- 使用 chunk 避免一次從 CSV 讀太多的檔案
- 程式的每一個階段，都幫我使用 Notebook 的不同段落來表示
- 建立有效的進度條
- 計算各項的分數 (F1 Score, Recall, Precision, Accuracy)

我將會使用下面的 Feature

```
    'Protocol', 'Timestamp', 'Flow Duration', 'Tot Fwd Pkts', 'Tot Bwd Pkts', 'TotLen Fwd Pkts',
    'TotLen Bwd Pkts', 'Fwd Pkt Len Max', 'Fwd Pkt Len Min', 'Fwd Pkt Len Mean', 'Fwd Pkt Len Std',
    'Bwd Pkt Len Max', 'Bwd Pkt Len Min', 'Bwd Pkt Len Mean', 'Bwd Pkt Len Std', 'Flow Byts/s',
    'Flow Pkts/s', 'Flow IAT Mean', 'Flow IAT Std', 'Flow IAT Max', 'Flow IAT Min', 'Fwd IAT Tot',
    'Fwd IAT Mean', 'Fwd IAT Std', 'Fwd IAT Max', 'Fwd IAT Min', 'Bwd IAT Tot', 'Bwd IAT Mean',
    'Bwd IAT Std', 'Bwd IAT Max', 'Bwd IAT Min', 'Fwd PSH Flags', 'Bwd PSH Flags', 'Fwd URG Flags',
    'Bwd URG Flags', 'Fwd Header Len', 'Bwd Header Len', 'Fwd Pkts/s', 'Bwd Pkts/s', 'Pkt Len Min',
    'Pkt Len Max', 'Pkt Len Mean', 'Pkt Len Std', 'Pkt Len Var', 'FIN Flag Cnt', 'SYN Flag Cnt',
    'RST Flag Cnt', 'PSH Flag Cnt', 'ACK Flag Cnt', 'URG Flag Cnt', 'CWE Flag Count', 'ECE Flag Cnt',
    'Down/Up Ratio', 'Pkt Size Avg', 'Fwd Seg Size Avg', 'Bwd Seg Size Avg', 'Fwd Byts/b Avg',
    'Fwd Pkts/b Avg', 'Fwd Blk Rate Avg', 'Bwd Byts/b Avg', 'Bwd Pkts/b Avg', 'Bwd Blk Rate Avg',
    'Subflow Fwd Pkts', 'Subflow Fwd Byts', 'Subflow Bwd Pkts', 'Subflow Bwd Byts', 'Init Fwd Win Byts',
    'Init Bwd Win Byts', 'Fwd Act Data Pkts', 'Fwd Seg Size Min', 'Active Mean', 'Active Std', 'Active Max',
    'Active Min', 'Idle Mean', 'Idle Std', 'Idle Max', 'Idle Min'
```

以下是程式主要的步驟

1. 載入 csv 檔
2. 將 csv 中 Label 有 "Benign" 的提取出來成單一的 CSV 檔。並且將其他非 "Benign" Label 的資料依照原始的天數 (CSV 來源檔檔名) 做相關的儲存。
3. 拿 "Benign" 的 CSV 做分割 Train set 和 Test sets
4. 將 Train Set 拿去做 Autoencoder 的訓練 (相關的參數你可以先設定，我之後會自己再做調整)，在要使用哪些 Feature 的部分，請留一個欄位讓我做設定
5. 拿 Test Sets 做測試 (Test Sets 是好流量，壞流量的部分請用非 "Benign" 的資料做測試)，輸出各項 Score
