# 文件描述

根目錄中的 [deal.py](/deal.py) 及 [count_protocol.py](/count_protocol.py) 主要處理透過 CSV 中所提供的資訊，在原始的 PCAP 中提取相對應的 Packets 出來。

[Model_Training](/Model_Training) 中所以存放的資料是在訓練模型所使用的程式。

裡面的 [Autoencoder](/Model_Training/Autoencoder) 資料夾中的所有檔案是與訓練 Autoencoder 模型相關的。詳情請看 [Readme](/Model_Training/Autoencoder/README.md)。

其餘文檔可以查詢 [HackMD](https://hackmd.io/@imdog) 所提供的內容。

# 相關資訊

## CIC-DDoS-2018-Protocol-Info

以下表格是用 [count_protocol.py](/count_protocol.py) 統計 CICDDoS2018 DataSet 中各張 CSV 中所有 Protocol 的數量結果 (單位為 Flows)。

| 檔案                                                 | TCP (Flows) | UDP (Flows) | 0       |
|----------------------------------------------------|-------------|-------------|---------|
| Friday-02-03-2018_TrafficForML_CICFlowMeter.csv    | 832492      | 202701      | 13382   |
| Friday-16-02-2018_TrafficForML_CICFlowMeter.csv    | 1048397     | 15          | 162     |
| Friday-23-02-2018_TrafficForML_CICFlowMeter.csv    | 740107      | 292401      | 16067   |
| Thuesday-20-02-2018_TrafficForML_CICFlowMeter.csv  | 5295644     | 2508972     | 144132  |
| Thursday-01-03-2018_TrafficForML_CICFlowMeter.csv  | 212899      | 111052      | 7149    |
| Thursday-15-02-2018_TrafficForML_CICFlowMeter.csv  | 684486      | 345524      | 18565   |
| Thursday-22-02-2018_TrafficForML_CICFlowMeter.csv  | 726422      | 305980      | 16173   |
| Wednesday-14-02-2018_TrafficForML_CICFlowMeter.csv | 829309      | 207384      | 11882   |
| Wednesday-21-02-2018_TrafficForML_CICFlowMeter.csv | 1044863     | 3651        | 61      |
| Wednesday-28-02-2018_TrafficForML_CICFlowMeter.csv | 385716      | 216615      | 10740   |
|                                                    |             |             |         |
| Total                                              | 11,800,335  | 4,194,295   | 238,313 |