import mariadb
from config import MARIADB_HOST, MARIADB_PORT, MARIADB_USER, MARIADB_PASSWORD, MARIADB_DATABASE

def connect_to_mariadb():
    """
    建立與 MariaDB 的連接
    :return: MariaDB 連接物件
    """
    try:
        conn = mariadb.connect(
            host = MARIADB_HOST,
            port = MARIADB_PORT,
            user = MARIADB_USER,
            password = MARIADB_PASSWORD,
            database = MARIADB_DATABASE
        )
        print(f"Successfully connected to MariaDB ({MARIADB_HOST}:{MARIADB_PORT} DB:{MARIADB_DATABASE}).")
        return conn
    except mariadb.Error as e:
        print(f"無法連線至 MariaDB，請檢查您的資料庫設定。錯誤訊息: {e}")
        return None

def check_connection(conn):
    """
    檢查 MariaDB 連線狀態，如果中斷則嘗試重新連線
    :param conn: MariaDB 連接物件
    :return: 有效的 MariaDB 連接物件
    """
    try:
        if conn is None:
            print("MariaDB 連線物件為空，嘗試建立新連線...")
            return connect_to_mariadb()
        conn.ping()
        return conn
    except mariadb.Error as e:
        print(f"檢測到 MariaDB 連線異常 ({e})，正在重新連線...")
        return connect_to_mariadb()
    except Exception as e:
        print(f"發生未預期錯誤 ({e})，嘗試重新連線...")
        return connect_to_mariadb()
    

def insert_data(conn, table, data):
    """
    將資料插入到指定的 MariaDB 表格中
    :param conn: MariaDB 連接物件
    :param table: 要插入資料的表格名稱
    :param data: 要插入的資料，應為字典格式 (如：{key1: value1, key2: value2})
    """
    try:
        cursor = conn.cursor()
        columns = ', '.join(data.keys())
        placeholders = ', '.join(['?'] * len(data))
        sql = f"INSERT INTO {table} ({columns}) VALUES ({placeholders})"
        cursor.execute(sql, tuple(data.values()))
        conn.commit()
        print("資料插入成功。")
        return cursor.lastrowid
    except mariadb.Error as e:
        print(f"插入資料時發生錯誤: {e}")