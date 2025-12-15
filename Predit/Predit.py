import config
import redis_tool
import mariadb_tool
import model_tool
import model_architectures
import torch
import time

if __name__ == "__main__":
    deveice = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = model_architectures.CNN_LSTM(num_classes=config.TOTAL_LABLE)
    model.load_state_dict(torch.load(config.MODEL_PATH, map_location = deveice))
    model.to(deveice)
    model.eval()

    rDB = redis_tool.connect_to_redis()
    mDB = mariadb_tool.connect_to_mariadb()
    while (1):
        # 從 Redis 取得一筆 HSET 資料
        key_name, hset = redis_tool.get_a_hset_data(rDB)
        print(f"Get Key : {key_name}")

        if hset is None:
            time.sleep(1)
            continue

        tensor = model_tool.load_images_from_hset(hset)

        # 跳過所有 tensor 不足 N_PKTS 張圖片的情況

        if config.CONVERT_MODE == "HAST_Two":
            if tensor.numel() == 0 or tensor.shape[1] != config.N_PKTS:
                continue
        elif config.CONVERT_MODE == "Flow2img4":
            if tensor.numel() == 0 or tensor.shape[1] != config.N_PKTS / 3:
                continue

        model_predit = model_tool.model_predit(tensor, model, deveice)

        model_tool.save_results(rDB, mDB, model_predit, hset)