CC=g++
TARGET=zpipe
LZ=-lz
# 替换成 zlib 的安装路径
SEARCH_PATH=

# 判断 SEARCH_PATH 的值是否存在，存在则定义新的变量 SEARCH_PARAM，如果为空则赋值 SEARCH_PARAM 为空
ifneq ($(SEARCH_PATH),)
SEARCH_PARAM=-I $(SEARCH_PATH)
else
SEARCH_PARAM=
endif

# 先调用 clean 删除原本生成的文件再生成
default: clean
	$(CC) -v -std=c++11 zpipe.cc main.cc $(SEARCH_PARAM) -lz -o $(TARGET)

clean:
	rm -f $(TARGET)
