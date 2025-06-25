import os
import torch
import torch.nn as nn
from PIL import Image
import torchvision.transforms as transforms

# 定義區
img_floder_path = "" # 請填入要預測的圖片資料夾路徑
full_model_path = "./LeNet_mnist_complete_model.pth" # 完整模型
# model_path = "./LeNet_mnist_model.pth" # 僅有權重的模型
label = ['Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Good', 'Bot_Attack',
         'Brute_Force_Web_Attack', 'Brute_Force_XSS_Attack', 'DDoS_FTP_Attack', 'DDoS_HOIC_Attack',
         'DDoS_LOIC_HTTP_Attack', 'DDoS_LOIC_UDP_Attack', 'DoS_GoldenEye_Attack', 'DoS_Hulk_Attack',
         'DoS_SlowHTTPTest_Attack', 'DoS_Slowloris_Attack', 'Infiltration_Attack', 'SQL_Injection',
         'SSH_Bruteforce_Attack']

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

class LeNet(nn.Module):
    def __init__(self, num_classes):
        super(LeNet, self).__init__()
        # LeNet-5 architecture for MNIST
        self.features = nn.Sequential(
            # C1: 卷積層 - 1@28x28 -> 6@24x24
            nn.Conv2d(1, 6, kernel_size=5),
            nn.Tanh(),
            # S2: 池化層 - 6@24x24 -> 6@12x12
            nn.AvgPool2d(kernel_size=2, stride=2),
            # C3: 卷積層 - 6@12x12 -> 16@8x8
            nn.Conv2d(6, 16, kernel_size=5),
            nn.Tanh(),
            # S4: 池化層 - 16@8x8 -> 16@4x4
            nn.AvgPool2d(kernel_size=2, stride=2),
        )

        self.classifier = nn.Sequential(
            # C5: 全連接層 - 16*4*4 -> 120
            nn.Linear(16 * 4 * 4, 120),
            nn.Tanh(),
            # F6: 全連接層 - 120 -> 84
            nn.Linear(120, 84),
            nn.Tanh(),
            # 輸出層 - 84 -> num_classes
            nn.Linear(84, num_classes),
        )

    def forward(self, x):
        x = self.features(x)
        x = x.view(x.size(0), -1)
        x = self.classifier(x)
        return x

# 在 LeNet 訓練的過程中，在 [2] 的 21, 22 行中有將 ubyte 的資料重新做縮放，所以需要使用 transforms.ToTensor() 來將資料先正規化後再丟給
# 後續的模型做處理 transforms.ToTensor() 同時也可以將 img 直接轉換到 tensor 形式的數值
preprocess = transforms.Compose([transforms.ToTensor()])

def predit(img_path, model):
    # deal_img = Image.open(img_path)
    # img = deal_img.load()
    # [img_x_size, img_y_size] = deal_img.size
    #
    # # pytorch 的預設是 (batch_size, channel, high, width)
    # img_tensor = torch.ones(1, 1, img_y_size, img_x_size)
    #
    # for w in range(0, img_x_size):
    #     for h in range(0, img_y_size):
    #         img_tensor[0, 0, h, w] = img[h, w]

    # 下面直接使用 torchvision 的套件幫我處理 PIL 輸入的 img
    img = Image.open(img_path)
    img_tensor = preprocess(img)

    # 加上 batch 的維度
    img_tensor = img_tensor.unsqueeze_(0)

    img_tensor = img_tensor.to(device)

    result = model(img_tensor)
    result_index = torch.argmax(result, dim=1).item()

    # print(f"{os.path.basename(img_path)} is {label[result_index]}")
    print(f"{os.path.basename(img_path)}, {label[result_index]}, {result_index}")

if __name__ == "__main__":
    # 在 pytorch 中要載入完整的模型，如果 version >= 2.6 的話 weight_only 預設會是 True
    model = torch.load(full_model_path, map_location = device, weights_only = False)

    # 載入僅有權重的模型
    # model = LeNet(num_classes=len(label))
    # model.load_state_dict(torch.load(model_path, map_location = device))

    model.to(device)

    model.eval()

    FileList = []
    for img_name in os.listdir(img_floder_path):
        path = os.path.join(img_floder_path, img_name)
        if path.endswith(".png"):
            FileList.append(os.path.join(img_floder_path, img_name))

    for filename in FileList:
        predit(filename, model)