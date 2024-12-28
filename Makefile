FLAGS=-std=c++17 -ggdb -O2 `pkg-config --cflags --libs opencv4` -Wno-unused-result

target: $(OBJS)
	g++ src/*.cpp $(FLAGS)
