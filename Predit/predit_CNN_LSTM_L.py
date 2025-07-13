import os
import torch
import torch.nn as nn
from PIL import Image
import torchvision.transforms as transforms

# 定義區
flow_floder_path = "" # 請填入要預測的流量資料夾路徑
# full_model_path = "" # 完整模型
model_path = "" # 僅有權重的模型
label = ['Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Bot_Attack',
         'Brute_Force_Web_Attack', 'Brute_Force_XSS_Attack', 'DDoS_LOIC_HTTP_Attack', 'DDoS_FTTP_Attack',
         'DDoS_HOIC_Attack', 'DDoS_LOIC_UDP_Attack', 'DoS_GoldenEye_Attack', 'DoS_Hulk_Attack',
         'DoS_SlowHTTPTest_Attack', 'DoS_Slowloris_Attack', 'Infiltration_Attack', 'SQL_Injection',
         'SSH_Bruteforce_Attack']
n_pkts = 6
m_bytes = 100

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# 定義模型架構
class SimpleCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(1, 32, 3, 1, 1), nn.ReLU(), nn.MaxPool2d(2),
            nn.Conv2d(32,64,3,1,1), nn.ReLU(), nn.MaxPool2d(2)
        )
        self.fc = nn.Linear(64*16*16, 128)
    def forward(self, x):
        x = self.features(x)
        x = x.view(x.size(0), -1)
        return self.fc(x)

class CNN_LSTM(nn.Module):
    def __init__(self, num_classes=26):
        super().__init__()
        self.cnns = nn.ModuleList([SimpleCNN() for _ in range(n_pkts)])
        self.lstm = nn.LSTM(input_size=128, hidden_size=64, batch_first=True)
        self.classifier = nn.Linear(64, num_classes)

    def forward(self, x):
        B = x.size(0)
        outs = []
        for i in range(n_pkts):
            outs.append(self.cnns[i](x[:,i]))
        seq = torch.stack(outs, dim=1)
        lstm_out, _ = self.lstm(seq)
        final = lstm_out[:, -1, :]
        return self.classifier(final)

# 後續的模型做處理 transforms.ToTensor() 同時也可以將 img 直接轉換到 tensor 形式的數值
preprocess = transforms.Compose([
    transforms.Resize((64, 64)), # 縮放圖片大小
    transforms.ToTensor(),  # 將灰階圖片轉為 [1, H, W]
])

def predit(FlowDir_path, model):

    img_tensors_list = [] # 建立一個列表來收集單張圖片的 Tensor
    # 確定圖片讀取順序，從 0.png 到 5.png
    img_paths = [os.path.join(FlowDir_path, f"{i}.png") for i in range(6)]

    if all(os.path.exists(p) for p in img_paths): # 確保 0~5 的 PNG 沒問題
       for img_path in img_paths:
        # 讀取並轉換為灰階
         img = Image.open(img_path).convert('L') 
        # 應用轉換 (Resize, ToTensor)
         img_tensor = preprocess(img) # 形狀會是 [1, 64, 64]
        # 加入到列表中
         img_tensors_list.append(img_tensor)

    #  使用 torch.stack 將 6 個 [1, 64, 64] 的 Tensor 堆疊成 [6, 1, 64, 64]
    stacked_imgs_tensor = torch.stack(img_tensors_list)

    #  使用 unsqueeze(0) 增加一個批次維度，變成 [1, 6, 1, 64, 64]
    #  這個 `final_tensor` 的格式就和模型訓練時的輸入完全一樣了
    final_tensor = stacked_imgs_tensor.unsqueeze(0)
    final_tensor = final_tensor.to(device)
    
    # 使用 with torch.no_grad() 在預測時可以節省記憶體並加速
    with torch.no_grad():
        result = model(final_tensor)
    
    result_index = torch.argmax(result, dim=1).item()
    predicted_label = label[result_index]

    # print(f"資料夾: {os.path.basename(FlowDir_path)}, 預測結果: {predicted_label}, 索引: {result_index}")

    # CSV 輸出
    print(f"{os.path.basename(FlowDir_path)}, {predicted_label}, {result_index}")

if __name__ == "__main__":
    # 在 pytorch 中要載入完整的模型，如果 version >= 2.6 的話 weight_only 預設會是 True
    # model = torch.load(full_model_path, map_location = device, weights_only = False)

    # 載入僅有權重的模型
    model = CNN_LSTM(num_classes=len(label))
    model.load_state_dict(torch.load(model_path, map_location = device))

    model.to(device)

    model.eval()

    FileList = []
    for flow_name in os.listdir(flow_floder_path): # 將 flow_name 輪著替換 flow_floder_path 內容
        path = os.path.join(flow_floder_path, flow_name)
        
        if (os.path.isdir(path) and os.listdir(path) !=[]): # 是資料夾且有資料才加入 FileList
         FileList.append(os.path.join(flow_floder_path, flow_name))

    for filename in FileList:
        predit(filename, model)