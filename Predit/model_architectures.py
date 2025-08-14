import torch
import torch.nn as nn

# 從 config 導入模型需要的常數
from config import N_PKTS

class SimpleCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(1,32,3,1,1), nn.ReLU(), nn.MaxPool2d(2),
            nn.Conv2d(32,64,3,1,1), nn.ReLU(), nn.MaxPool2d(2)
        )
        self.fc = nn.Linear(64*16*16, 128)
    def forward(self, x):
        x = self.features(x)
        x = x.view(x.size(0), -1)
        return self.fc(x)

class CNN_LSTM(nn.Module):
    def __init__(self, num_classes):
        super().__init__()
        self.cnns = nn.ModuleList([SimpleCNN() for _ in range(N_PKTS)])
        self.lstm = nn.LSTM(input_size=128, hidden_size=64, num_layers=2, batch_first=True, dropout=0.5)
        self.classifier = nn.Linear(64, num_classes)

    def forward(self, x):
        B = x.size(0)
        outs = []
        for i in range(N_PKTS):
            outs.append(self.cnns[i](x[:,i]))
        seq = torch.stack(outs, dim=1)
        lstm_out, _ = self.lstm(seq)
        final = lstm_out[:, -1, :]
        return self.classifier(final)