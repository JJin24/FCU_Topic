# 定義區 (model)
FLOW_FLODER_PATH = ""  # 請填入要預測的流量資料夾路徑
# FULL_MODEL_PATH = "" # 完整模型
MODEL_PATH = "" # 僅有權重的模型
TOTAL_LABLE = # 總類別數量 (不含 Other)
N_PKTS = 
M_BYTES = 
THRESHOLD =  # 預測機率的閾值
GOOD_INDEX = 

# 定義區 (redis_tool)
REDIS_HOST = ''
REDIS_PORT = 
REDIS_DB =

# 定義區 (mariadb_tool)
MARIADB_HOST = ''
MARIADB_PORT = 
MARIADB_USER = ''
MARIADB_PASSWORD = ''
MARIADB_DATABASE = ''