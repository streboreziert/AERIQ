#!/bin/bash
# Setup Raspberry Pi Zero W2 as WiFi Access Point for AERIQ
# SSID: AERIQ | Password: aeriq1234 | IP: 192.168.4.1
#
# Run once:  sudo bash setup_ap.sh
set -e

echo "=== AERIQ Access Point Setup ==="

# Create AP connection using NetworkManager
sudo nmcli connection delete AERIQ-AP 2>/dev/null || true
sudo nmcli connection add type wifi ifname wlan0 con-name AERIQ-AP \
    autoconnect yes ssid AERIQ \
    wifi.mode ap wifi.band bg \
    wifi-sec.key-mgmt wpa-psk wifi-sec.psk "aeriq1234" \
    ipv4.method shared ipv4.addresses 192.168.4.1/24

sudo nmcli connection up AERIQ-AP

echo ""
echo "Access Point 'AERIQ' is running at 192.168.4.1"
echo ""
echo "Next steps:"
echo "  1. pip3 install -r requirements.txt"
echo "  2. python3 app.py"
echo "  3. Dashboard: http://192.168.4.1:8000"
