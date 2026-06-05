CXX      = g++
CXXFLAGS = -O2 -Wall -std=c++11 `pkg-config --cflags sdl2 cairo`
LDFLAGS  = `pkg-config --libs sdl2 cairo` -lm

SRCDIR   = src
OBJDIR   = obj
TARGET   = SynPaint

SRCS     = $(wildcard $(SRCDIR)/*.cpp)
OBJS     = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(TARGET)
