CC=g++
MAKE=make
TARGET=zpipe
LZ=-lz
# 替换成 zlib 的安装路径
SEARCH_PATH=

# 判断 SEARCH_PATH 的值是否存在，存在则定义新的变量 SEARCH_PARAM，如果为空则赋值 SEARCH_PARAM 为空
ifneq ($(SEARCH_PATH),)
SEARCH_PARAM=-I $(SEARCH_PATH)
LIB_Z_ARCHIVE_FILE=$(SEARCH_PATH)/libz.a
else
SEARCH_PARAM=
LIB_Z_ARCHIVE_FILE=
endif

# 先调用 clean 删除原本生成的文件再生成
all: clean zpipe.o
	$(CC) -v -std=c++11 zpipe.o main.cc $(LIB_Z_ARCHIVE_FILE) $(SEARCH_PARAM) $(LZ) -o $(TARGET) \
	&& rm zpipe.o

zpipe.o:
	$(CC) -c zpipe.cc

.PHONY: clean
clean:
	rm -f $(TARGET)
