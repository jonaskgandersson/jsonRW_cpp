all:
	g++ -std=c++11 -g -O -Wall -Weffc++ -pedantic test/main.cpp source/jsonRW.cpp source/ascii_num.c -o build/jsonTest
