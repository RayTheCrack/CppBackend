# 防止make clean作用到名为clean的文件上

.PHONY: clean

# 替代代码段 $(Name)使用

CFLAGS = -Wall -g -O2

src = message.cpp demo.cpp

objs = demo.o message.o

tar = app

# app: demo.cpp message.cpp
# 	g++ demo.cpp message.cpp -o app 
# 可用，但不建议这样写，因为此时任意cpp文件修改，都会导致全体cpp文件重新编译，工程项目中，通常采用以下写法，保证每次只编译已修改文件

$(tar) : $(objs)
	g++ $(CFLAGS) $(objs) -o $@

%.o : %.cpp
	g++ $(CFLAGS) -c $< -o $@

# $< 指分号后，$@ 指分号前

# demo.o: demo.cpp
# 	g++ -c demo.cpp

# message.o: message.cpp
# 	g++ -c message.cpp

clean: # make clean 清除编译产生的中间文件
	rm -rf *.o app