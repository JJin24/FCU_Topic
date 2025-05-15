# 使用 Claude 3.7 建立
# 目的是希望將 CICFlowMeter 所建立的 csv 中所有的 Protocol 內容顯示出來
# 以了解需要撰寫在 deal.py 中 Protocol 的類別
# 此程式也可以修改以查詢其他欄位的分佈

import pandas as pd
import os

def count_protocols(csv_path):
    try:
        # 讀取 CSV 檔案
        df = pd.read_csv(csv_path)

        # 檢查是否包含 protocol 欄位
        if 'Protocol' in df.columns:
            # 計算每種 protocol 的出現次數
            protocol_counts = df['Protocol'].value_counts()
            print(f"檔案: {os.path.basename(csv_path)}")
            print("Protocol 種類及數量:")
            print(protocol_counts)
            print(f"總共有 {len(protocol_counts)} 種不同的 protocol")
            return protocol_counts
        else:
            print(f"錯誤: '{csv_path}' 中沒有 'protocol' 欄位")
            # 顯示可用的欄位名稱
            print(f"可用的欄位: {df.columns.tolist()}")
            return None
    except Exception as e:
        print(f"讀取檔案時發生錯誤: {e}")
        return None


if __name__ == "__main__":
    # 預設使用目前資料夾中的 CSV 檔案
    # 您可以修改此路徑指向實際的 CSV 檔案
    csv_path = input("請輸入 CSV 檔案路徑 (按 Enter 使用預設路徑): ")

    if not csv_path:
        print("請提供有效的 CSV 檔案路徑")
        csv_files = [f for f in os.listdir('.') if f.endswith('.csv')]
        if csv_files:
            print("目前目錄中的 CSV 檔案:")
            for i, file in enumerate(csv_files, 1):
                print(f"{i}. {file}")
            selection = input("請選擇檔案編號 (1-{0}): ".format(len(csv_files)))
            try:
                index = int(selection) - 1
                if 0 <= index < len(csv_files):
                    csv_path = csv_files[index]
                else:
                    print("無效的選擇")
                    exit(1)
            except ValueError:
                print("請輸入有效數字")
                exit(1)
        else:
            print("目前目錄下沒有找到 CSV 檔案")
            exit(1)

    count_protocols(csv_path)