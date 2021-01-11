LDLIBS = -lSDL2 -lm -lGLX_mesa
CXXFLAGS = -Wall -std=c++17 -fno-exceptions -g

example : example.cpp ../bmpimage.cpp 
	$(CXX) $(CXXFLAGS) -o $@ example.cpp ../bmpimage.cpp $(LDLIBS)

.PHONY: clean
clean:
	rm example *.o
