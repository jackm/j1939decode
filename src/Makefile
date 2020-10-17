all: cJSON j1939decode link

cJSON: cJSON.c cJSON.h
	gcc -g -c -std=c99 -o cJSON.o cJSON.c

j1939decode: j1939decode.cpp j1939decode.h
	g++ -g -c -std=c++11 -o j1939decode.o j1939decode.cpp

link: cJSON.o j1939decode.o
	g++ -g -o j1939decode cJSON.o j1939decode.o