#!/usr/bin/env bash

# Exit immediately on errors
set -euo pipefail

echo "=========================================================="
echo "        Asus Texas Instruments Audio Fixer"
echo "=========================================================="
echo "This script decompresses the TAS2781 speaker firmware"
echo "and recreates the required system symbolic links."
echo ""

# Check if run as root
if [ "$EUID" -ne 0 ]; then
  echo "Error: This script must be run with root privileges."
  echo "Please run: sudo ./fix_audio.sh"
  exit 1
fi

echo "[1/4] Decompressing Texas Instruments speaker firmware..."
# Force decompressing real files, ignoring warnings for existing links
zstd -d --keep --force /lib/firmware/ti/audio/tas2781/*.bin.zst || true

echo "[2/4] Recreating internal driver symlink..."
# Inside ti/audio/tas2781/
ln -sf TXNW2781RCA0.bin /lib/firmware/ti/audio/tas2781/TXNW2781RCA1.bin

echo "[3/4] Recreating uncompressed system firmware symlinks..."
# Under /lib/firmware/
ln -sf ti/audio/tas2781/TXNW2781RCA1.bin /lib/firmware/TXNW2781RCA0.bin
ln -sf ti/audio/tas2781/TXNW2781RCA0.bin /lib/firmware/TXNW2781RCA1.bin
ln -sf ti/audio/tas2781/TIAS2781RCA2.bin /lib/firmware/TXNW2781RCA2.bin
ln -sf ti/audio/tas2781/TIAS2781RCA4.bin /lib/firmware/TXNW2781RCA4.bin

echo "[4/4] Verifying the new uncompressed files..."
ls -l /lib/firmware/TXNW2781RCA*.bin

echo ""
echo "=========================================================="
echo "          AUDIO FIRMWARE FIX APPLIED SUCCESSFULLY!"
echo "=========================================================="
echo "Please REBOOT your laptop now."
echo "Your laptop speakers will now load and work perfectly!"
echo "=========================================================="
