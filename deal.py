import pandas as pd
from scapy.all import *
import os
import glob
import datetime
import time

def main():
    # 設定檔案和目錄路徑
    csv_file = "D:\\cicddos-2018\\2018-02-21\\test.csv"  # 請替換成您的CSV檔案名稱
    pcap_directory = "D:\\cicddos-2018\\2018-02-21\\pcap\\pcap"        # PCAP檔案所在目錄
    output_directory = "D:\\cicddos-2018\\2018-02-21\\output"  # 輸出目錄
    
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
            label = str(row.get('Label', 'unknown'))  # 如果沒有Label欄位，使用'unknown'
            
            print(f"\n處理第 {index+1}/{len(df)} 筆資料:")
            print(f"Protocol={protocol}, DstPort={dst_port}, Timestamp={timestamp_str}")
            
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
                    packets = rdpcap(pcap_file)
                    print(f"處理PCAP檔案: {pcap_file}，包含 {len(packets)} 個封包")
                except Exception as e:
                    print(f"無法讀取 {pcap_file}: {e}")
                    continue
                
                for packet in packets:
                    if IP not in packet:
                        continue
                    
                    # 檢查協議
                    if ((protocol == 6 and TCP in packet) or
                        (protocol == 17 and UDP in packet) or
                        (protocol == 1 and ICMP in packet)):
                        
                        # 檢查目的端口 (ICMP無端口)
                        port_match = False
                        if protocol == 1 and ICMP in packet:
                            port_match = True
                        elif TCP in packet and packet[TCP].dport == dst_port:
                            port_match = True
                        elif UDP in packet and packet[UDP].dport == dst_port:
                            port_match = True
                        
                        if port_match:
                            # 檢查時間戳 (允許1秒誤差)
                            time_diff = abs(packet.time - timestamp_seconds)
                            if time_diff <= 1.0:
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