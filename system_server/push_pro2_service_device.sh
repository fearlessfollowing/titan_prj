#!/bin/bash
adb connect 192.168.55.1
adb shell killall monitor
adb shell killall pro2_service
adb push pro2_service /usr/local/bin/
adb shell chmod +x /usr/local/bin/pro2_service
adb shell reboot
