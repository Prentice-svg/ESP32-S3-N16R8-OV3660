#!/bin/bash
#
# GB2312 Font File Setup Helper Script
# This script helps download or prepare GB2312 font files for the ESP32 project
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FONT_DIR="${SCRIPT_DIR}/font_resources"
SD_MOUNT_POINT="/Volumes/SD"  # macOS default, adjust for your OS

echo "=================================================="
echo "ESP32 Chinese Font Setup Helper"
echo "=================================================="
echo ""

# Check if running on macOS or Linux
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
    echo "Detected: macOS"
    # For macOS, SD card typically mounts at /Volumes/SD*
    SD_MOUNT_POINT=$(ls -d /Volumes/SD* 2>/dev/null | head -1)
    if [ -z "$SD_MOUNT_POINT" ]; then
        echo "SD Card not found. Please insert and mount SD card first."
        exit 1
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
    echo "Detected: Linux"
    # For Linux, SD card typically mounts at /mnt or /media
    SD_MOUNT_POINT=$(mount | grep -E "/mnt|/media" | awk '{print $3}' | head -1)
    if [ -z "$SD_MOUNT_POINT" ]; then
        echo "SD Card not found. Please mount SD card first."
        exit 1
    fi
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "SD Card mount point: $SD_MOUNT_POINT"
echo ""

# Create font directory on SD card
echo "Creating font directory on SD card..."
mkdir -p "${SD_MOUNT_POINT}/font"
echo "✓ Font directory created: ${SD_MOUNT_POINT}/font"
echo ""

# Check for existing font files
echo "Checking for existing font files..."
if ls "${SD_MOUNT_POINT}/font"/*.fon 1> /dev/null 2>&1; then
    echo "Found existing font files:"
    ls -lh "${SD_MOUNT_POINT}/font"/*.fon
    echo ""
    read -p "Do you want to replace them? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f "${SD_MOUNT_POINT}/font"/*.fon
        echo "Removed existing font files."
    else
        echo "Keeping existing font files."
        exit 0
    fi
fi

echo ""
echo "Font File Options:"
echo "1. Use pre-built fonts (download from GitHub)"
echo "2. Manual setup (you provide the .fon file)"
echo "3. Skip (no font file setup)"
echo ""
read -p "Select option (1-3): " option

case $option in
    1)
        echo ""
        echo "Downloading fonts from kaixindelele/ssd1306-MicroPython-ESP32-Chinese..."
        echo ""

        # We would download from the GitHub repo
        # Since this is a helper script, we'll show instructions instead

        echo "To download fonts manually, visit:"
        echo "https://github.com/kaixindelele/ssd1306-MicroPython-ESP32-Chinese"
        echo ""
        echo "Download the font files (GB2312-12.fon, GB2312-16.fon, etc.)"
        echo "and place them in: ${SD_MOUNT_POINT}/font/"
        echo ""
        echo "Or use wget/curl to download directly:"
        echo "  wget -O GB2312-16.fon <URL>"
        echo ""
        ;;
    2)
        echo ""
        echo "Manual Font Setup"
        echo "=================="
        echo ""
        echo "Please provide your .fon file:"
        read -p "Enter path to .fon file: " font_file

        if [ -f "$font_file" ]; then
            size=$(stat -f%z "$font_file" 2>/dev/null || stat -c%s "$font_file" 2>/dev/null)
            filename=$(basename "$font_file")

            echo "Copying $filename ($(($size/1024))KB)..."
            cp "$font_file" "${SD_MOUNT_POINT}/font/$filename"
            echo "✓ Font file copied successfully"

            # Detect font size
            echo ""
            echo "Detected file size: $size bytes"
            echo "This likely corresponds to:"

            if [ $size -gt 100000 ] && [ $size -lt 150000 ]; then
                echo "  → 12x12 font (GB2312-12.fon)"
            elif [ $size -gt 200000 ] && [ $size -lt 250000 ]; then
                echo "  → 16x16 font (GB2312-16.fon)"
            elif [ $size -gt 450000 ] && [ $size -lt 550000 ]; then
                echo "  → 24x24 font (GB2312-24.fon)"
            elif [ $size -gt 800000 ] && [ $size -lt 900000 ]; then
                echo "  → 32x32 font (GB2312-32.fon)"
            else
                echo "  → Unknown size (may need verification)"
            fi
        else
            echo "Error: File not found: $font_file"
            exit 1
        fi
        ;;
    3)
        echo "Skipping font setup."
        echo "Note: You can add fonts later by copying .fon files to ${SD_MOUNT_POINT}/font/"
        ;;
    *)
        echo "Invalid option"
        exit 1
        ;;
esac

echo ""
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "1. Safely eject the SD card"
echo "2. Insert into ESP32"
echo "3. Update your code to use font_init(\"/font/GB2312-16.fon\")"
echo ""
echo "For more information, see: CHINESE_FONT_GUIDE.md"
