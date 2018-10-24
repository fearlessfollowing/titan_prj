#!/bin/bash
adb connect 192.168.55.1
adb push out/pro2_service   /usr/local/bin/
#adb push out/power_manager   /usr/local/bin/
adb shell chmod +x /usr/local/bin/pro2_service
#adb shell chmod +x /usr/local/bin/power_manager

adb shell killall pro2_service
