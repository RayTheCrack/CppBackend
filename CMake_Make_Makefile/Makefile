# app: demo.cpp message.cpp
# 	g++ demo.cpp message.cpp -o app 
# 可用，但不建议这样写，因为此时任意cpp文件修改，都会导致全体cpp文件重新编译，工程项目中，通常采用以下写法，保证每次只编译已修改文件

app: demo.o message.o
	g++ demo.o message.o -o app

demo.o: demo.cpp
	g++ -c demo.cpp

message.o: message.cpp
	g++ -c message.cpp
	