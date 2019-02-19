#coding=utf-8 
import ctypes 
from ctypes import *   
ll = ctypes.cdll.LoadLibrary   
lib = ll("./libprint.so")  
str='abcdefg'
p=c_char_p(str)
rst=lib.print(p, len(str))

