######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  UnixSocket.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2019年02月26日    skymixos                V1.0.11         创建文件
######################################################################################################

import sys
import socket
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
NO_CONTROLLER       = "No controller"


# 全局回调接口(用于处理Client请求)
gCallbackEntery = None


def genSendDataFromDict(req):
    Info('genSendDataFromDict conent {}'.format(req))
    sendContent = json.dumps(req)
    magicHead = int_to_bytes(HEAD_MAGIC)
    dataLen = int_to_bytes(len(sendContent))
    bytesContent = str_to_bytes(sendContent)
    return join_byte_list((magicHead, dataLen, bytesContent))


def genSendDataFromStr(data):
    # Info('genSendDataFromStr conent {}'.format(data))
    sendContent = data
    magicHead = int_to_bytes(HEAD_MAGIC)
    dataLen = int_to_bytes(len(sendContent))
    bytesContent = str_to_bytes(sendContent)
    return join_byte_list((magicHead, dataLen, bytesContent))



# Magic + len
# {"state":"error", "error": "recv header error"}
# 
def genTransError(desc):
    hdrErrResult = OrderedDict()
    hdrErrResult['state'] = 'error'
    hdrErrResult['error'] = desc
    return hdrErrResult


class MyUnixSocket:
    def __init__(self):
        pass
    
    def genSendDataFromDictEx(self, cmd, data):
        sendContentDict = OrderedDict({'cmd': cmd, 'parameters': data})
        sendContent = json.dumps(sendContentDict)
        magicHead = int_to_bytes(HEAD_MAGIC)
        dataLen = int_to_bytes(len(sendContent))
        bytesContent = str_to_bytes(sendContent)
        return join_byte_list((magicHead, dataLen, bytesContent))

    def recvData(self, socket):
        header = socket.recv(COMMON_HDR_LEN)
        Info('-------> recvData len is {}'.format(header))

        magicHead = bytes_to_int(header, 0)
        contentLen = bytes_to_int(header, 4)
        if magicHead != HEAD_MAGIC or contentLen <= 0:
            Error("Recv head error -----")
        else:
            Info('contentLen is {}'.format(contentLen))
            readContent = socket.recv(contentLen)
            content = bytes_to_str(readContent)
                    
            Print('recv content content {}'.format(content))
            return content
        
        return None

#
# MyUnixServerHandler - 用于处理连接请求
#
class MyUnixServerHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # Info("-----++ MyUnixServerHandler: handle ++----------------")
        header = self.request.recv(COMMON_HDR_LEN)
        if (len(header)) <= 0:
            Info('---------------->>>> header len is 0')
        else:
            magicHead = bytes_to_int(header, 0)
            contentLen = bytes_to_int(header, 4)
            if magicHead != HEAD_MAGIC or contentLen <= 0:
                Info("--++ MyUnixServerHandler: handle Recv head error -----")
                self.request.sendall(genSendDataFromDict(genTransError(PARSE_HDR_ERROR)))            
            else:
                # Info('contentLen is {}'.format(contentLen))
                readContent = self.request.recv(contentLen)
                if len(readContent) != contentLen:
                    Info("---+ MyUnixServerHandler: handle read len != contentLen ++-------")
                    self.request.sendall(genSendDataFromDict(genTransError(READ_CONTENT_ERROR)))            
                else:
                    content = bytes_to_str(readContent)
                    # Info('recv content content {}'.format(content))
                    reqDict = jsonstr_to_dic(content)
                    if check_dic_key_exist(reqDict, "name"):                
                        if UnixSocketServer.getCallBackEntry() != None:
                            if reqDict['name'] in UnixSocketServer.getCallBackEntry().systemServerReqHandler:
                                result = UnixSocketServer.getCallBackEntry().systemServerReqHandler[reqDict['name']](reqDict) 
                            else:
                                result = genTransError(UNSUPPORT_CMD)
                        else:
                            result = genTransError(NO_CONTROLLER)
                    else:
                        result = genTransError(PARAM_INVALID)

                    # if (reqDict['name'] == config._GET_SET_CAM_STATE):
                    #     Info('Rsponse: {}'.format(result))

                    self.request.sendall(genSendDataFromStr(result))


#
# UnixSocketServer - 用于接收Client发送的请求(包括system_server)
#
class UnixSocketServer(MyUnixSocket):

    callBackEntry = None

    def __init__(self, controller):
        self.WEB_SERVER_SOCKET_PATH = "/dev/socket/web_server"
        UnixSocketServer.callBackEntry = controller    

    def startServer(self):
        if os.path.exists(self.WEB_SERVER_SOCKET_PATH):
            os.unlink(self.WEB_SERVER_SOCKET_PATH)

        with socketserver.UnixStreamServer(self.WEB_SERVER_SOCKET_PATH, MyUnixServerHandler) as server:
            server.serve_forever()

    def stopServer(self):
        pass

    @classmethod
    def getCallBackEntry(cls):
        return UnixSocketServer.callBackEntry


class UnixSocketServerHandle(Thread):
    def __init__(self, name, controller):
        super().__init__()
        self._name = name
        self._socketServer = UnixSocketServer(controller)
        Info('-++--> UnixSocketServerHandle constructor <-++--')

    def run(self):  
        Info('----------------> Starting Unix socket server..')
        self._socketServer.startServer()


#
# UnixSocketClient - 用于给system_server发送异步消息
# 
class UnixSocketClient(MyUnixSocket):
    def __init__(self):
        self.SYSTEM_SERVER_PATH = "/dev/socket/system_server"
        self.sendLock = Semaphore()
        self._socket = None


    def connectServer(self):
        try:
            self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self._socket.connect(self.SYSTEM_SERVER_PATH)
        except OSError as e:
            Info("Create Unix Socket Error: {}".format(e))
            self._socket = None
        return self._socket


    def disconnectServer(self):
        Info("disconnect socket server")
        if self._socket != None:
            self._socket.close()



    #################################################################################################
    # 方法名称: sendAsyncNotify
    # 方法功能: 发送异步通知
    # 入口参数: 
    #   cmd - 命令ID
    #   data - 命令对应的参数
    # 返 回 值: 成功返回大于0; 失败返回错误码
    #################################################################################################
    def sendAsyncNotify(self, cmd, data = None):
        self.sendLock.acquire()
        sendAsyncResult = True
        content = None

        if data != None:
            content = self.genSendDataFromDictEx(cmd, data)
        else:
            content = genSendDataFromDict(cmd)

        Info('------> sendAsyncNotify conent {}'.format(content))

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


    #################################################################################################
    # Head： 0xDEADBEEF + Content_Len 
    # Content: {“cmd": rsp_cmd, "parameters"： {}}
    #
    # 发送同步请求
    ##################################################################################################
    def sendSyncRequest(self, cmd, data = None):
        content = None
        requestResult = None

        self.sendLock.acquire()
        
        if data != None:
            content = genSendDataFromDictEx(cmd, data)
        else:
            content = genSendDataFromDict(cmd)
        
        if self.connectServer() != None:
            try:
                self._socket.sendall(content)
                Info('-----------> send Request over, recv result here...')
                requestResult = self.recvData(self._socket)
            except InterruptedError as e:
                Info('sendAsyncNotify got InterruptedError {}'.format(e))
            except Exception as e:
                Info('sendsyncNotify got Except {}'.format(e))

            finally:
                self.disconnectServer()
        else:
            Info("Connect system_server for sync request failed")

        self.sendLock.release()
        
        return requestResult
