import os
import platform
import sys


def main_func(dev_ip, dst_bin):
    print("----- push binary program to device -------")
    print("destination device ip: ", dev_ip)
    print("push program: ", dst_bin)
    
    cmd_connect = "adb connect " + dev_ip
    print("Adb connect device ", dev_ip)
    os.system(cmd_connect)

    push_bin = "adb push out/" + dst_bin + " /usr/local/bin"
    print("Adb push ", dst_bin)
    os.system(push_bin)

    chmod_bin = "adb shell chmod +x /usr/local/bin/" + dst_bin
    os.system(chmod_bin)

    restart_bin = "adb shell killall " + dst_bin
    os.system(restart_bin)


if len(sys.argv) == 1:
    print("Usage: python3.5 push_bin2dev.py <ip> <binary>")
    print("Example: python3.5 push_bin2dev.py 192.168.55.1 out/update_check")
else:
    main_func(sys.argv[1], sys.argv[2])