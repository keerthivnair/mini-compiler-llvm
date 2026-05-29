#!/usr/bin/env bash

# Exit immediately on errors
set -euo pipefail

GRUB_FILE="/etc/default/grub"
BACKUP_FILE="/etc/default/grub.bak"

echo "=========================================================="
echo "        Western Digital NVMe SSD Stability Fixer"
echo "=========================================================="
echo "This script disables aggressive power management latency transitions"
echo "on WD SN5000S/SN750/SN850 drives to prevent random read-only locks."
echo ""

# Check if run as root
if [ "$EUID" -ne 0 ]; then
  echo "Error: This script must be run with root privileges."
  echo "Please run: sudo ./fix_nvme_stability.sh"
  exit 1
fi

echo "[1/4] Backing up GRUB configuration..."
cp "$GRUB_FILE" "$BACKUP_FILE"
echo "      Backup saved to: $BACKUP_FILE"

echo "[2/4] Modifying kernel parameters..."
if grep -q "nvme_core.default_ps_max_latency_us" "$GRUB_FILE"; then
  echo "      Stability parameter is already present. No patching required."
else
  # Inject the parameter into GRUB_CMDLINE_LINUX_DEFAULT
  sed -i 's/\(GRUB_CMDLINE_LINUX_DEFAULT="[^"]*\)"/\1 nvme_core.default_ps_max_latency_us=0"/' "$GRUB_FILE"
  echo "      Parameter successfully injected into GRUB config."
fi

echo "[3/4] Rebuilding GRUB boot files..."
update-grub

echo ""
echo "=========================================================="
echo "          STABILITY FIX APPLIED SUCCESSFULLY!"
echo "=========================================================="
echo "Please REBOOT your laptop to activate the changes."
echo "Your NVMe SSD will now remain rock-solid under compilation."
echo "=========================================================="
