import redis
from config import REDIS_HOST, REDIS_PORT, REDIS_DB

def connect_to_redis(host = REDIS_HOST, port = REDIS_PORT, db = REDIS_DB):
    """
    建立與 Redis 的連接
    :param host: Redis 伺服器的主機名稱或 IP 地址
    :param port: Redis 伺服器的端口號
    :param db: 使用的資料庫編號
    :return: Redis 連接物件
    """
    try:
        r = redis.Redis(host = host, port = port, db = db)
        print(f"Successfully connected to Redis ({host}:{port} DB:{db}).")
        return r
    except redis.exceptions.ConnectionError as e:
        print(f"無法連線至 Redis，請檢查您的 Redis 伺服器是否正在運行。錯誤訊息: {e}")
        return None

def get_a_hset_data(r):
    """
    從 Redis 中取得一筆 HSET 資料
    :param r: Redis 連接物件
    :return: HSET 的鍵名和資料，或 None 如果沒有找到
    """
    try:
        for key in r.scan_iter():
            print(f"Checking key: {key}")
            if r.type(key).decode("utf-8") == 'hash':
                print(f"Get Key : {key}")
                hset_data = r.hgetall(key)
                r.delete(key) # 刪除已取得的 HSET 資料
                return key, hset_data
            r.delete(key) # 如果不是 HSET，則刪除該鍵
        return None, None
    except Exception as e:
        print(f"發生錯誤: {e}")
        return None, None
    
def exit_redis_connection(r):
    """
    關閉 Redis 連接
    :param r: Redis 連接物件
    """
    if r:
        r.close()
        print("Redis 連接已關閉。")
    else:
        print("沒有有效的 Redis 連接可關閉。")