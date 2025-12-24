import http
import requests
import json
from config import API_HOST, TIME_LIMIT, TOTAL_LABLE
import time
from datetime import *

last_time = datetime.strptime('1970-01-01 00:00:00', "%Y-%m-%d %H:%M:%S")

def send_alert_notification(label, dst_ip, dst_port, timestamp, score):
    # 說明 last_time 為全域變數
    global last_time
    data = {}

    print("\n查詢告警 " + label + " " + dst_ip + ":" + dst_port + " 相關資訊...\n=======================")

    if label != TOTAL_LABLE + 1:
        dst_ip_bak = dst_ip
        dst_ip = '::ffff:' + dst_ip
        hostname = requests.get(API_HOST + '/host/name/' + dst_ip).json()
        label_name = requests.get(API_HOST + '/labelList/id/' + str(label)).json()
        hostname = hostname[0]['name'] if hostname and len(hostname) > 0 else "未知主機"
        label_name = label_name[0]['name'] if label_name and len(label_name) > 0 else "Unknown (未知流量)"
        data = {
            'label': label_name,
            'hostName': hostname,
            'dst_ip': dst_ip_bak + ':' + dst_port,
            'timestamp': timestamp,
            'score': score
        }
        requests.get(API_HOST + '/host/setMalHost/' + hostname)
        print(data)
        requests.post(API_HOST + '/notify/alert', json = data)
    else:
        if(datetime.now() - last_time > timedelta(seconds = TIME_LIMIT)):
            last_time = datetime.now()
            dst_ip_bak = dst_ip
            dst_ip = '::ffff:' + dst_ip
            hostname = requests.get(API_HOST + '/host/name/' + dst_ip).json()
            label_name = requests.get(API_HOST + '/labelList/id/' + str(label)).json()
            hostname = hostname[0]['name'] if hostname and len(hostname) > 0 else "未知主機"
            label_name = label_name[0]['name'] if label_name and len(label_name) > 0 else "Unknown"
            data = {
                'label': label_name,
                'hostName': hostname,
                'dst_ip': dst_ip_bak + ':' + dst_port,
                'timestamp': timestamp,
                'score': score
            }

            requests.get(API_HOST + '/host/setGoodHost/' + hostname)
            print(data)
            requests.post(API_HOST + '/notify/alert', json = data)