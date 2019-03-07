import socket
import sys
import config
import json
import os
import socketserver

from threading import Thread  
from threading import Semaphore
from collections import OrderedDict
from util.str_util import *
from util.ins_util import *
from util.ins_log_util import *



COMMON_HDR_LEN      = 8
HEAD_MAGIC          = 0xDEADBEEF
PARSE_HDR_ERROR     = "parse header error"
READ_CONTENT_ERROR  = "read len error"
UNSUPPORT_CMD       = "Unsupport command"
PARAM_INVALID       = "Invalid Parameter"


class UnixSocketClient:
    def __init__(self):
        self.SYSTEM_SERVER_PATH = "/dev/socket/web_server"
        self.sendLock = Semaphore()
        self._socket = None

    def genSendData(self, req):
        sendContent = json.dumps(req)
        magicHead = int_to_bytes(HEAD_MAGIC)
        dataLen = int_to_bytes(len(sendContent))
        bytesContent = str_to_bytes(sendContent)
        return join_byte_list((magicHead, dataLen, bytesContent))


    def genSendDataEx(self, cmd, data):
        sendContentDict = OrderedDict({'cmd': cmd, 'parameters': data})
        sendContent = json.dumps(sendContentDict)
        magicHead = int_to_bytes(HEAD_MAGIC)
        dataLen = int_to_bytes(len(sendContent))
        bytesContent = str_to_bytes(sendContent)
        return join_byte_list((magicHead, dataLen, bytesContent))


    def recvRspData(self):
        header = self._socket.recv(COMMON_HDR_LEN)
        magicHead = bytes_to_int(header, 0)
        contentLen = bytes_to_int(header, 4)
        if magicHead != HEAD_MAGIC or contentLen <= 0:
            print("Recv head error -----")
            return None
        else:
            readContent = self._socket.recv(contentLen)
            if len(readContent) != contentLen:
                print('------> Error: read len not equal ind len')
                return None
            else:
                print('-------------- read rsp ok -------------------')
                content = bytes_to_str(readContent)
                print(content)
                return jsonstr_to_dic(content)

    def connectServer(self):
        try:
            self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self._socket.connect(self.SYSTEM_SERVER_PATH)
        except OSError as e:
            self._socket = None
        return self._socket


    def disconnectServer(self):
        print("disconnect socket server")
        if self._socket != None:
            self._socket.close()


    #
    # Head： 0xDEADBEEF + Content_Len 
    # Content: {“cmd": rsp_cmd, "parameters"： {}}
    #
    # 发送同步请求
    def sendSyncRequest(self, cmd, data = None):
        content = None
        requestResult = None

        self.sendLock.acquire()
        
        if data != None:
            content = self.genSendDataEx(cmd, data)
        else:
            content = self.genSendData(cmd)
        
        if self.connectServer() != None:
            try:
                self._socket.sendall(content)
                requestResult = self.recvRspData()
                if requestResult != None and requestResult['state'] == 'done':
                    print('------------ query success ----------')
            except InterruptedError as e:
                print('sendAsyncNotify got InterruptedError ', e)
            except Exception as e:
                print('sendsyncNotify got Except ', e)

            finally:
                self.disconnectServer()
        else:
            print("Connect system_server for sync request failed")

        self.sendLock.release()
        
        return requestResult



cli = UnixSocketClient()
if cli.connectServer() != None:
    print('connect server: /dev/socket/web_server success')
    cli.sendSyncRequest(OrderedDict({"name": "camera._queryGpsStatus"}))
else:
    print('connect server: /dev/socket/web_server failed')


