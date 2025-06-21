CC=gcc
# CFLAGS: 編譯器旗標 (警告、最佳化等)
CFLAGS=-Wall -Wextra -O3
# LDFLAGS: 連結器旗標 (要連結的函式庫)
LDFLAGS=-lpcap

# 預設目標 'all' 依賴於我們的最終執行檔
all: Flow2NMimg

# 執行檔 'Flow2NMimg' 依賴於來源檔 'Flow2NMimg.c'
Flow2NMimg: Flow2NMimg.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 'clean' 目標，用於清除編譯產生的檔案
clean:
	rm -f Flow2NMimg

.PHONY: all clean