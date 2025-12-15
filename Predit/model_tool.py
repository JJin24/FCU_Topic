import io
import torch
import numpy as np
from PIL import Image
from torchvision import transforms
import config
import mariadb_tool
import json
import predit_api
import ipaddress

def load_images_from_hset(hset: dict):
    """
    從已經從 Redis 取出的 HSET (Python 字典) 中，載入多張圖片並轉換為一個批次 Tensor。
    
    :param hset: 一個 Python 字典，包含了從 Redis HSET 取出的資料。
                  鍵為 string，值為 bytes。
    :return: 一個包含所有圖片的批次 Tensor，shape 為 [N, C, H, W]。
    """
    # 步驟 1: 定義通用的影像轉換流程
    transform = transforms.Compose([
        transforms.ToTensor()  # 轉換為 Tensor (會將像素值標準化到 [0, 1])
    ])
    
    processed_tensors = []  # 使用 Python list 來暫存處理好的單張圖片 Tensor

    if config.CONVERT_MODE == "HAST_Two":
        img_names = [f"{i}.png".encode("utf-8") for i in range(config.N_PKTS)]
    elif config.CONVERT_MODE == "Flow2img4":
        img_names = [f"{i}.png".encode("utf-8") for i in range(int(config.N_PKTS / 3))]

    binary_data = hset.items()  # 取得 HSET 中的所有鍵值對

    # 步驟 2: 迭代處理每一張圖片
    for name in img_names:
        binary_data = hset.get(name)
        
        # 錯誤處理：檢查圖片是否存在
        if binary_data is None:
            print(f"警告：在 HSET 中找不到欄位 '{name}'，將跳過此圖片。")
            continue
            
        try:
            if config.CONVERT_MODE == "HAST_Two":
                img = Image.open(io.BytesIO(binary_data)).convert('L')  # 轉換為灰階圖
            elif config.CONVERT_MODE == "Flow2img4":
                img = Image.open(io.BytesIO(binary_data)).convert('RGB') # 轉換為 RGB 圖
            
            # 應用轉換，得到一張圖片的 Tensor (shape: [C, H, W])
            tensor_img = transform(img)
            
            processed_tensors.append(tensor_img)
        except Exception as e:
            print(f"警告：處理欄位 '{name}' 時發生錯誤: {e}，將跳過此圖片。")
            return torch.empty(0)  # 如果有任何圖片處理失敗，回傳一個空的 Tensor

    # 步驟 3: 將 list 中的所有 Tensor 堆疊成一個批次 Tensor
    # 如果 list 為空 (所有圖片都載入失敗)，回傳一個空的 Tensor
    if not processed_tensors:
        # 您可以決定回傳 None 或一個空 Tensor
        return torch.empty(0) 

    # torch.stack 會在第 0 維度上增加一個新的維度 (batch dimension)
    # [C, H, W], [C, H, W], ... -> [N, C, H, W]
    batch_tensor = torch.stack(processed_tensors, dim = 0)
    batch_tensor = batch_tensor.unsqueeze(0)  # 增加一個批次維度，變成 [1, N, C, H, W]
    
    return batch_tensor

def model_predit(tensor: torch.Tensor, model: torch.nn.Module, device: torch.device):
    """
    使用模型對輸入的 Tensor 進行預測。
    
    :param tensor: 輸入的批次 Tensor，shape 為 [1, N, C, H, W]。
    :param model: 已經載入權重的 PyTorch 模型。
    :param device: PyTorch 的裝置 (CPU 或 GPU)。
    :return: 模型的預測結果。
    """
    model.eval()  # 設定模型為評估模式
    tensor = tensor.to(device)  # 將 Tensor 移動到指定裝置
    
    with torch.no_grad():  # 在預測時不需要計算梯度
        result = model(tensor)
    
    return result

def save_results(rDB, mDB, result, hset):
    """
    將模型的預測結果儲存到 Redis 中。
    
    :param rDB: Redis 連接物件。
    :param mDB: MariaDB 連接物件。
    :param result: 模型的預測結果，應該是 Tensor 或其他可序列化的格式。
    :param hset: 需要操作的 HSET 資料。
    """
    try:
        # 使用 softmax 取得機率分布
        prob = torch.softmax(result, dim = 1)
        score, pred_class = torch.max(prob, dim = 1)

        score = score.item()
        pred_class = pred_class.item()
        print(f"Predicted class: {pred_class}, Score: {score}")

        # 檢查預測分數是否超過閾值
        if score < config.THRESHOLD:
            pred_class = config.TOTAL_LABLE + 1

        # 從 Redis 取出的原始 hset 值是 bytes，需要解碼成字串
        metadata = hset.get('metadata'.encode("utf-8")).decode("utf-8")
        metadata = json.loads(metadata) # 轉換成字典格式

        print(f"Metadata: {metadata}")

        flow_info = {
            'timestamp' : metadata.get('timestamp'),
            'src_ip' : metadata.get('s_ip'),
            'dst_ip' : metadata.get('d_ip'),
            'src_port' : metadata.get('s_port'),
            'dst_port' : metadata.get('d_port'),
            'protocol' : metadata.get('protocol')
        }

        if ipaddress.ip_address(metadata.get('s_ip')).version == 4:
            flow_info['src_ip'] = '::ffff:' + metadata.get('s_ip')
            flow_info['dst_ip'] = '::ffff:' + metadata.get('d_ip')

        last_id = mariadb_tool.insert_data(mDB, 'flow', flow_info)
        print("Flow data inserted into MariaDB.")
        print(f"Type of last_id: {type(last_id)}, score: {type(score)}, pred_class: {type(pred_class)}")

        if pred_class != config.GOOD_INDEX:
            alert_data = {
                'id': last_id,
                'pcap': hset.get('pcap_data'.encode("utf-8")),
                'score': str(score),
                'label': str(pred_class)
            }
            mariadb_tool.insert_data(mDB, 'alert_history', alert_data)
            predit_api.send_alert_notification(str(pred_class), metadata.get('d_ip'), metadata.get('d_port'),
                                    metadata.get('timestamp'), str(score))

    except Exception as e:
        print(f"儲存結果到 MariaDB 時發生錯誤: {e}")