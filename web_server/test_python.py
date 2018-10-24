import json

test_bytes = b'{"name":"camera._queryState", "state":"done","results":{"state":"record","version":"1.2.2 build-201702271155  ","origin":{"width":3200,"height":2400,"framerate":25,"bitrate":30720,"mime":"h264"},"record":{"saveOrigin":"true","url":"/mnt/media_rw/58AA-9F25/VID_2017_02_27_09_06_22"}}}'

def bytes_to_str(b):
    # assert isinstance(b,bytes)
    return b.decode(encoding='utf-8')

def str_to_bytes(str):
    # assert isinstance(str,basestring)
    return str.encode(encoding='utf-8')

def jsonstr_to_dic(str):
    return str.encode(encoding='utf-8')

print('strlen test_bytes {}'.format(len(test_bytes)))

str_test = bytes_to_str(test_bytes)
print("str_test is {}".format(str_test))

json_test = json.loads(str_test)
print("json_test is {}".format(json_test))