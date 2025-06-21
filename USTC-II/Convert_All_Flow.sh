#!/bin/bash
# Program:
#   This script will convert all the flow files (.pcap) in the input directory
#   to the output directory with the specified conversion tool.
# Usage:
#   ./Convert_All_Flow.sh <input_directory> <output_directory> <n_pkt> <m_byte>

convert_tool="" # 替換為實際的轉換工具路徑

input_top_folder=$1
output_top_folder=$2
n_pkt=$3
m_byte=$4

if [ -z "$input_top_folder" ] || [ -z "$output_top_folder" ] || [ -z "$n_pkt" ] || [ -z "$m_byte" ]; then
    echo "Usage: $0 <input_directory> <output_directory> <n_pkt> <m_byte>"
    exit 1
fi

# 原始這邊是使用 ls -d 來列出下面的子目錄，但如果遇到 " " 的話
# 參數就會有問題，丟給 Gemini 後，它提供下面的方式來得到子目錄的名稱
# 使用 find 命令來列出所有子目錄 (只會找到一層深度的目錄)。接著透過
# 管道將每個子目錄的路徑傳遞給 while 迴圈進行處理。
find "$input_top_folder" -mindepth 1 -maxdepth 1 -type d | while read -r input_folder; do
    # 從完整路徑中提取子目錄名稱
    gci=$(basename "$input_folder")
    
    output_folder="$output_top_folder/$gci"
    
    mkdir -p "$output_folder"
    
    $convert_tool "$input_folder" "$output_folder" "$n_pkt" "$m_byte"

done