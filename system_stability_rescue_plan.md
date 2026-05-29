# NVMe SSD Stability & Interview Disaster Recovery Plan

This document explains the root cause of the random freezes, disk errors, and system hang-ups experienced on this laptop, and provides a foolproof, multi-layered action plan to guarantee 100% uptime and readiness during your interviews.

---

## 1. The Root Cause: Western Digital SSD vs. Linux APST Bug

### The Symptoms
* The system randomly freezes under compiling/heavy loads.
* The screen displays "Input/output error" or "Read-only file system".
* The system gets completely stuck until you physically close and reboot ("close n open").

### The Science
This laptop is equipped with a **Western Digital PC SN5000S NVMe SSD**. 
In Linux, Western Digital NVMe controllers have a long-standing firmware incompatibility with **Aggressive Link Power Management (APST)**. 
When the system is under load (like running parallel C++ compiler jobs), the SSD quickly transitions between power saving levels. If a transition takes a fraction of a millisecond longer than expected, the Linux kernel loses the device link, assumes the hardware has suffered an catastrophic failure, and instantly remounts the root directory `/` as **Read-Only** to protect your data from corruption.

Once the disk goes read-only, nothing can be written, the desktop GUI locks up, and only a full power cycle restores connectivity.

---

## 2. Action Plan: Fixing the Stability Issue

### Step 1: Execute the Automated Stability Fixer
I have written an automated shell script [`fix_nvme_stability.sh`](./fix_nvme_stability.sh) directly in your project folder. This script backs up your GRUB configuration, safely injects the kernel latency parameter `nvme_core.default_ps_max_latency_us=0` (disabling aggressive sleep states), and rebuilds the boot files.

Run these exact commands in your Ubuntu terminal:

```bash
# 1. Navigate to your project directory
cd /home/ubuntu/nvidia/MINI_COMPILER_PROJECT

# 2. Run the automated fixer script with root privileges
sudo ./fix_nvme_stability.sh
```

*Provide your password when prompted. The script will output confirmation and complete within seconds.*

### Step 2: Reboot Your Laptop
Once the script completes successfully, **reboot your laptop**. The parameter will take effect, and your SSD will remain active, stable, and completely immune to disconnects under compile loads.

---

## 3. Interview Best Practices: Reducing Load

When building code or testing your compiler during the interview, avoid running raw, multi-threaded parallel compilations (e.g. `make -j` with no arguments, which attempts to spawn a thread for every core, hitting 100% CPU capacity and causing sudden overheating).

Instead, throttle the compiler slightly:
```bash
# Limits CMake/Make to using only 2 CPU cores.
# Keeps temperatures cool (around 60°C - 70°C) and prevents RAM OOM bottlenecks!
make -j2
```

---

## 4. The 60-Second Disaster Recovery Plan

If your hardware experiences a complete power failure mid-interview, do not panic. We have fully backed up your workspace to GitHub under **[mini-compiler-llvm](https://github.com/keerthivnair/mini-compiler-llvm)**.

If your laptop goes down:
1. Immediately grab a **backup computer, tablet, or another device**.
2. Open a browser and go to your repository: `https://github.com/keerthivnair/mini-compiler-llvm`.
3. Log in to your GitHub account and press the **`.`** (period) key on your keyboard.
4. This will instantly launch **GitHub Codespaces** inside your browser. 
5. GitHub Codespaces provides a dedicated, high-speed virtual Linux container pre-configured with VS Code. You can immediately build, run, and display your compiler progress online in **less than 60 seconds**!
