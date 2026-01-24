CC=gcc
# CFLAGS: 編譯器旗標 (警告、最佳化等)
CFLAGS=-Wall -Wextra -O3
# LDFLAGS: 連結器旗標 (要連結的函式庫)
LDFLAGS=-lpcap -lpthread -lhiredis -pthread -lcurl

# 預設目標 'all' 依賴於我們的最終執行檔
all: Flow2img Flow2img_II Capture_Flow RealTime_Flow2img_Convert Flow_Capture_v7

# 執行檔 'Flow2img' 依賴於來源檔 'Flow2img.c'
Flow2img: Flow2img.c de_addr_noise.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

Flow2img_II: Flow2img_II.c de_addr_noise.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

Capture_Flow: capture_Flow.c flow_tracker.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

RealTime_Flow2img_Convert: HAST_Two.c Flow2img4.c thread_pool.c main_server.c redis_handler.c de_addr_noise.c stb_image_write_impl.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

Flow_Capture_v7: flow_capturev7.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 'clean' 目標，用於清除編譯產生的檔案
clean:
	rm -f Flow2img Capture_Flow Flow2img_II RealTime_Flow2img_Convert Flow_Capture_v7
.PHONY: all clean
