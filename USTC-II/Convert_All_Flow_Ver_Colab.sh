#!/bin/bash
# Program:
#   This script iterates through subdirectories of a given input folder,
#   and converts all flow files (.pcap) within each subdirectory to a 
#   corresponding subdirectory in the output folder.
# Gemini 更動調整，避免壟餘的程式碼，並增加錯誤處理。

# --- Configuration ---
# The path to the conversion tool
convert_tool="./Flow2img" 

# n_pkt="6"
# m_byte="100"

# set -e: Exit immediately if a command exits with a non-zero status.
# set -u: Treat unset variables as an error when substituting.
# set -o pipefail: The return value of a pipeline is the status of the last command 
#                  to exit with a non-zero status, or zero if no command exited
#                  with a non-zero status.
set -euo pipefail

# --- Functions ---
# Function to display usage information
usage() {
    echo "Usage: $0 [option] <input_top_folder> <output_top_folder> <parameter_1> <parameter_2>"
    echo "This script converts .pcap files in subdirectories of <input_top_folder> to <output_top_folder>."
    echo ""
    echo "Options:"
    echo "  -f, --flow, -1, --hast-1, HAST-I    Use HAST-I conversion (flow-based)."
    echo "  -p, --packet, -2, --hast-2, HAST-II Use HAST-II conversion (packet-based)."
    echo "  -h, --help                          Display this help message."
    echo ""
    echo "Example:"
    echo "  $0 -1 ./my_pcaps ./my_images"
}

# --- Main Script Logic ---

# Check if -h or --help is passed
if [[ "${1-}" == "-h" || "${1-}" == "--help" ]]; then
    # If the tool has its own help, show it. Otherwise, show our usage.
    if [[ -x "$convert_tool" ]]; then
        "$convert_tool" -h
    else
        echo "Conversion tool '$convert_tool' not found or not executable."
        echo ""
        usage
    fi
    exit 0
fi

# Check for the correct number of arguments
if [[ $# -ne 5 ]]; then
    echo "Error: Invalid number of arguments."
    usage
    exit 1
fi

# Assign arguments to variables for clarity
option="$1"
input_top_folder="$2"
output_top_folder="$3"
parameter_1="$4"
parameter_2="$5"

# Validate that the input directory exists
if [[ ! -d "$input_top_folder" ]]; then
    echo "Error: Input directory '$input_top_folder' does not exist."
    exit 1
fi

# Validate that the conversion tool exists and is executable
if [[ ! -x "$convert_tool" ]]; then
    echo "Error: Conversion tool '$convert_tool' not found or not executable."
    exit 1
fi

# Determine the arguments for the conversion tool based on the option
case "$option" in
    -f|--flow|-1|--hast-1|"HAST-I")
        echo "Mode: HAST-I (flow-based) selected."
        # The `find` loop will handle the rest of the arguments
        ;;
    -p|--packet|-2|--hast-2|"HAST-II")
        echo "Mode: HAST-II (packet-based) selected."
        # The `find` loop will handle the rest of the arguments
        ;;
    *)
        echo "Error: Invalid option '$option'."
        usage
        exit 1
        ;;
esac

echo "Input directory:  $input_top_folder"
echo "Output directory: $output_top_folder"
echo "---"

# Find all immediate subdirectories in the input folder and process them
# The `find ... | while read` structure is excellent for handling names
# with spaces or special characters correctly.
find "$input_top_folder" -mindepth 1 -maxdepth 1 -type d | while read -r input_subdir; do
    # Extract the base name of the subdirectory (e.g., "Test" from "./123/Test")
    subdir_name=$(basename "$input_subdir")
    
    # Construct the corresponding output subdirectory path
    output_subdir="$output_top_folder/$subdir_name"
    
    echo "Processing: '$input_subdir' -> '$output_subdir'"
    
    # Create the output subdirectory, -p ensures it doesn't fail if it already exists
    mkdir -p "$output_subdir"
    
    # Execute the conversion tool with the correct arguments for the chosen mode
    if [[ "$option" == "-f" || "$option" == "--flow" || "$option" == "-1" || "$option" == "--hast-1" || "$option" == "HAST-I" ]]; then
        "$convert_tool" "$option" "$input_subdir" "$output_subdir" "$parameter_1"
    else # HAST-II mode
        "$convert_tool" "$option" "$input_subdir" "$output_subdir" "$parameter_1" "$parameter_2"
    fi
done

echo "---"
echo "Conversion complete."