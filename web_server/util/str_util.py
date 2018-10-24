
def bytes_to_str(b):
    # assert isinstance(b,bytes)
    return b.decode(encoding='utf-8')

def str_to_bytes(str):
    # assert isinstance(str,basestring)
    return str.encode(encoding='utf-8')

def int_to_bytes(n):
    b = bytearray([0, 0, 0, 0])  # init
    b[3] = n & 0xFF
    n >>= 8
    b[2] = n & 0xFF
    n >>= 8
    b[1] = n & 0xFF
    n >>= 8
    b[0] = n & 0xFF

    # Return the result or as bytearray or as bytes (commented out)
    ##return bytes(b)  # uncomment if you need
    return b

def bytes_to_int(b, offset):
    n = (b[offset + 0] << 24) | (b[offset + 1] << 16) | (b[offset + 2] << 8) | b[offset + 3]
    return n

def unify_float(f,num = 2):
    return round(f,num)

def join_str_list(list):
    if list is None:
        raise AssertionError('join_str_list None')
    return ''.join(list)

def join_byte_list(list):
    if list is None:
        raise AssertionError('join_byte_list None')
    return b''.join(list)

def str_exits(base,key):
    if base.find(key) != -1:
        return True
    else:
        return False

def str_start_with(base,key):
    if base.find(key) == 0:
        return True
    else:
        return False


