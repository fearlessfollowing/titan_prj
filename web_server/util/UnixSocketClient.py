######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  UnixSocketClient.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2019年02月26日    skymixos                V1.0.11         客户端断开时，给camerad发送MODULE_POWER_OFF消息
######################################################################################################

import sys
import socket
import config
import json

from threading import Semaphore
from collections import OrderedDict
from util.str_util import *


class UnixSocketClient:
    def __init__(self):
        self.SYSTEM_SERVER_PATH = "/dev/socket/system_server"
        self.sendLock = Semaphore()
        self._socket = None

    def connectServer(self):
        try:
            self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        except OSError as e:
            Info("Create Unix Socket Error: {}".format(e))
            self._socket = None
        
        return self._socket


    def disconnectServer(self):
        Info("disconnect socket server")
        if self._socket != None:
            self._socket.close()


    def genSendData(self, cmd, data):
        sendContentDict = OrderedDict({'cmd': cmd, 'parameters': data})
        sendContent = json.dumps(sendContentDict)
        magicHead = int_to_bytes(0xDEADBEEF)
        dataLen = int_to_bytes(len(sendContent))
        bytesContent = str_to_bytes(sendContent)
        return join_byte_list((magicHead, sendContent, bytesContent))


    def recvData(self, socket):
        header = socket.recv(HEAD_LEN)
        magicHead = bytes_to_int(header, 0)
        contentLen = bytes_to_int(header, 4)
        if magicHead != 0xBEADBEEF or contentLen <= 0:
            Error("Recv head error -----")
        else:
            Info('contentLen is {}'.format(content_len))
            readContent = socket.recv(contentLen)
            content = bytes_to_str(readContent)
                    
            Print('recv content content {}'.format(content))
            return jsonstr_to_dic(content)
        
        return None


    # 方法名称: sendAsyncNotify
    # 方法功能: 发送异步通知
    # 入口参数: 
    #   cmd - 命令ID
    #   data - 命令对应的参数
    # 返 回 值: 成功返回大于0; 失败返回错误码
    def sendAsyncNotify(self, cmd, data):
        self.sendLock.acquire()
        sendAsyncResult = True
        content = self.genSendData(cmd, data)
        Info('sendAsyncNotify conent {}'.format(sendContent))

        if self.connectServer() != None:
            try:
                self._socket.sendall(content)
            except InterruptedError as e:
                Info('sendAsyncNotify got InterruptedError {}'.format(e))
                sendAsyncResult = False
            except Exception as e:
                Info('sendAsyncNotify got Except {}'.format(e))
                sendAsyncResult = False

            finally:
                self.disconnectServer()
        else:
            Info("Create connect to system_server failed")
            sendAsyncResult = False

        self.sendLock.release()            
        return sendAsyncResult


    #
    # Head： 0xDEADBEEF + Content_Len 
    # Content: {“cmd": rsp_cmd, "parameters"： {}}
    #
    # 发送同步请求
    def sendSyncRequest(self, cmd, data, callback):
        self.sendLock.acquire()
        sendSyncResult = True
        content = self.genSendData(cmd, data)

        if self.connectServer() != None:
            try:
                self._socket.sendall(content)
                rspData = self.recvData(self._socket)
                if rspData != None and callback != None:
                    callback(rspData)
                else:
                    Info("---> rspData is None or needn't callback")
            except InterruptedError as e:
                Info('sendAsyncNotify got InterruptedError {}'.format(e))
                sendSyncResult = False            
            except Exception as e:
                Info('sendsyncNotify got Except {}'.format(e))
                sendSyncResult = False

            finally:
                self.disconnectServer()
        else:
            Info("Connect system_server for sync request failed")
            sendSyncResult = False
        self.sendLock.release()
        
        return sendSyncResult
