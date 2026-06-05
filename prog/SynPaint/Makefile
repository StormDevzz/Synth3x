CXX      = g++
CXXFLAGS = -O2 -Wall -std=c++11 `pkg-config --cflags sdl2 cairo`
LDFLAGS  = `pkg-config --libs sdl2 cairo` -lm

TARGET   = SynPaint

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
