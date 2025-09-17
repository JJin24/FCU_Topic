import http
import requests
import json
from config import API_HOST, TIME_LIMIT
import time
from datetime import *

last_time = datetime.strptime('1970-01-01 00:00:00', "%Y-%m-%d %H:%M:%S")

def send_alert_notification(label, dst_ip, timestamp, score):
    # 說明 last_time 為全域變數
    global last_time

    if(datetime.now() - last_time > timedelta(seconds = TIME_LIMIT)):
        last_time = datetime.now()
        dst_ip = '::ffff:' + dst_ip
        hostname = requests.get(API_HOST + '/host/name/' + dst_ip).json()
        label_name = requests.get(API_HOST + '/labelList/id/' + str(label)).json()
        hostname = hostname[0]['name']
        label_name = label_name[0]['name']
        data = {
            'label': label_name,
            'hostName': hostname,
            'dst_ip': dst_ip,
            'timestamp': timestamp,
            'score': score
        }

        print(data)
        requests.post(API_HOST + '/notify/alert', json = data)