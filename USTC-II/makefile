CC=gcc
# CFLAGS: 編譯器旗標 (警告、最佳化等)
CFLAGS=-Wall -Wextra -O3
# LDFLAGS: 連結器旗標 (要連結的函式庫)
LDFLAGS=-lpcap

# 預設目標 'all' 依賴於我們的最終執行檔
all: Flow2img Capture_Flow

# 執行檔 'Flow2img' 依賴於來源檔 'Flow2img.c'
Flow2img: Flow2img.c de_addr_noise.c de_addr_noise.h stb_image_write.h
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

Capture_Flow: capture_Flow.c uthash.h flow_tracker.c flow_tracker.h
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 'clean' 目標，用於清除編譯產生的檔案
clean:
	rm -f Flow2img Capture_Flow

.PHONY: all clean