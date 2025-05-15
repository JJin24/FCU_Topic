# 此程式目的是想將 CICFlowMeter 所建立的 CSV 的每一個 Flow
# 從 PCAP 裡面提取出。由於在 CICDDoS2018 中五元組僅剩下 2
# 元組而已 (dst_port, Protocol)，所以將會透過此兩個資訊加上
# Timestamp + Flow Duration 資訊將 Flow 從 PCAP 中找到
# 並提取出來。

import pandas as pd
from scapy.all import *
import os
import glob
import datetime
import time
from scapy.layers.inet import TCP, UDP

def main():
    # 設定檔案和目錄路徑
    csv_file = ""  # 請替換成您的CSV檔案名稱
    pcap_directory = "" # PCAP檔案所在目錄
    output_directory = "" # 輸出目錄

    # 建立輸出目錄
    os.makedirs(output_directory, exist_ok=True)

    # 讀取CSV檔案
    try:
        df = pd.read_csv(csv_file)
        print(f"CSV檔案讀取成功，共 {len(df)} 筆資料")
        print(f"欄位: {list(df.columns)}")
    except Exception as e:
        print(f"讀取CSV檔案時出錯: {e}")
        return

    # 找出所有PCAP檔案
    pcap_files = glob.glob(os.path.join(pcap_directory, "*.pcap"))
    print(f"找到 {len(pcap_files)} 個PCAP檔案")

    # 處理每筆flow資料
    for index, row in df.iterrows():
        try:
            protocol = int(row['Protocol'])
            timestamp_str = row['Timestamp']
            dst_port = int(row['Dst Port'])
            flow_duration = int(row['Flow Duration'] / 1000000 + 1)  # 時間在 csv 紀錄的是 ms，這邊將它轉成 s (無條件進位)
            label = str(row.get('Label', 'unknown'))  # 如果沒有Label欄位，使用'unknown'

            print(f"\n處理第 {index+1}/{len(df)} 筆資料:")
            print(f"Protocol={protocol}, DstPort={dst_port}, Timestamp={timestamp_str}, FlowDuration={flow_duration}s, Label={label}")

            # 解析時間戳
            try:
                # 嘗試不同的時間格式
                formats = ["%d/%m/%Y %H:%M:%S"]
                timestamp_dt = None

                for fmt in formats:
                    try:
                        timestamp_dt = datetime.datetime.strptime(timestamp_str, fmt)
                        break
                    except ValueError:
                        continue

                if timestamp_dt is None:
                    print(f"無法解析時間戳: {timestamp_str}")
                    continue

                timestamp_seconds = time.mktime(timestamp_dt.timetuple())
                if "." in timestamp_str:  # 如果有毫秒
                    timestamp_seconds += float("0." + timestamp_str.split(".")[-1])

            except Exception as e:
                print(f"解析時間戳時出錯: {e}")
                continue

            # 搜尋匹配的封包
            matching_packets = []
            flow_info = {'dst_ip': None, 'dport': dst_port, 'src_ip': None, 'sport': None}

            for pcap_file in pcap_files:
                try:
                    packets = rdpcap(pcap_file) # 將 PCAP 的內容轉換到可讀的格式 (最花時間的地方)
                    print(f"處理PCAP檔案: {pcap_file}，包含 {len(packets)} 個封包")
                except Exception as e:
                    print(f"無法讀取 {pcap_file}: {e}")
                    continue

                # 以下為範例 packet 的值：
                # <Ether  dst=02:e5:07:52:a7:a4 src=02:a3:6a:4e:81:58 type=IPv4 |
                # <IP  version=4 ihl=5 tos=0x0 len=40 id=33437 flags=DF frag=0 ttl=47 proto=tcp chksum=0x2275 src=151.101.34.202 dst=172.31.64.111 |
                # <TCP  sport=http dport=52649 seq=2919710215 ack=73547580 dataofs=5 reserved=0 flags=FA window=60 chksum=0xb33 urgptr=0 |>>>
                for packet in packets:
                    if IP not in packet:
                        continue

                    # 先計算時間差，檢查是否超出搜尋範圍 (Decimal('28800') 是 UTC-8)
                    current_time = packet.time - Decimal('28800')
                    max_find_time = current_time + Decimal(flow_duration)

                    # 如果已經超過最大時間範圍，就不用繼續處理此 pcap 檔案
                    if packet.time > max_find_time + 3:
                        print(f"已超出搜尋時間範圍，跳過剩餘封包")
                        found_time_exceeded = True
                        break

                    # 檢查協議
                    if ((protocol == 6 and packet[IP].proto == 6) or
                        (protocol == 17 and packet[IP].proto == 17) or
                        (protocol == 1 and packet[IP].proto == 1)):

                        # 檢查目的端口 (ICMP無端口)
                        port_match = False
                        if protocol == 1 and ICMP in packet:
                            port_match = True
                        elif packet[IP].proto == 6 and (packet[TCP].dport == dst_port or packet[TCP].sport == dst_port):
                            port_match = True
                        elif packet[IP].proto == 17 and (packet[UDP].dport == dst_port or packet[UDP].sport == dst_port):
                            port_match = True

                        if port_match:
                            # 裡面 timestamp 紀錄時間的問題，目前是將裡面的時間當作 UTC，但是在紀錄上面，轉換成 UTC-8
                            current_time = packet.time - Decimal('28800')
                            # 檢查時間戳
                            time_diff = abs(current_time - Decimal(timestamp_seconds))
                            if time_diff <= flow_duration + 3: # 批配的時間要在 flow_duration 的時間內 (多 3 秒的冗餘)
                                matching_packets.append(packet)

                                # 記錄第一個匹配封包的資訊
                                if flow_info['dst_ip'] is None:
                                    flow_info['dst_ip'] = packet[IP].dst
                                    flow_info['src_ip'] = packet[IP].src

                                    if TCP in packet:
                                        flow_info['sport'] = packet[TCP].sport
                                    elif UDP in packet:
                                        flow_info['sport'] = packet[UDP].sport
                                    else:
                                        flow_info['sport'] = 0

            if not matching_packets:
                print(f"找不到匹配的封包")
                continue

            print(f"找到 {len(matching_packets)} 個匹配封包")

            # 儲存匹配的封包
            time_str = timestamp_dt.strftime("%Y%m%d_%H%M%S")
            output_filename = f"{flow_info['dst_ip']}_{flow_info['dport']}_{flow_info['src_ip']}_{flow_info['sport']}_{time_str}_{label}.pcap"
            output_path = os.path.join(output_directory, output_filename)

            wrpcap(output_path, matching_packets)
            print(f"已保存匹配封包到: {output_path}")

        except Exception as e:
            print(f"處理資料時出錯: {e}")
            import traceback
            traceback.print_exc()

    print("\n處理完成!")

if __name__ == "__main__":
    main()