CXX?=clang++
LINKER=clang++
CFLAGS?=-g -std=c++11 -I/usr/local/include/SDL2 -DGLEW_STATIC
LDFLAGS?=-g -std=c++11 -framework OpenGL -lSDL2 -lSDL2main -lGLEW -DGLEW_STATIC

all: open.gl

clean:
	rm -rf open.gl *.o

open.gl: main.o
	$(LINKER) -o open.gl main.o $(LDFLAGS) 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

