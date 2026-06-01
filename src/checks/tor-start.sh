#!/bin/bash
# Synth3x-Anon — Lazy Tor start (saves ~30MB RAM until needed)
echo "Starting Tor transparent proxy..."
mkdir -p /var/lib/tor /var/log/tor /run
chown 100:100 /var/lib/tor /var/log/tor 2>/dev/null
/usr/bin/tor -f /etc/tor/torrc --runasdaemon 0 &
echo "Tor: PID $!"
echo "Run 'checks-all' to verify."
