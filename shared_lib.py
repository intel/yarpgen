import ctypes

# 라이브러리 로드
yarpgen_function = ctypes.CDLL('./libyarpgen.so')

# 함수 프로토타입 정의
yarpgen_function.gen_yarpgen.argtypes = (ctypes.c_int, ctypes.POINTER(ctypes.c_char_p))
yarpgen_function = yarpgen_function.gen_yarpgen

def make_param(str_list):
    c_string_array = (ctypes.c_char_p * len(str_list))()
    for i, string in enumerate(str_list):
        c_string_array[i] = bytes(string, 'utf-8')
    return len(str_list), c_string_array

# 테스트
if __name__ == "__main__":
    test_strings = ["yarpgen", "--std=c", "-o", "out"]
    ret = yarpgen_function(*make_param(test_strings))
    print(ret)