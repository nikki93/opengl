CXX?=clang++
LINKER=clang++
CFLAGS?=-g -std=c++11 -DGLEW_STATIC
LDFLAGS?=-g -std=c++11 -lsfml-graphics -lsfml-window -lsfml-system \
	 -lGLEW -DGLEW_STATIC

all: open.gl

clean:
	rm -rf open.gl *.o

open.gl: main.o
	$(LINKER) -o open.gl main.o $(LDFLAGS) 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

