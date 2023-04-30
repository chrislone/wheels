CC=g++
TARGET=zpipe
LZ=-lz
# 替换成 zlib 的安装路径
SEARCH_PATH=

# 先调用 clean 删除原本生成的文件再生成
default: clean
	$(CC) -v -std=c++11 zpipe.cc main.cc $(SEARCHPATH) -lz -o $(TARGET)

clean:
	rm -f $(TARGET)
