CC = g++
CFLAGS = -I/usr/local/include -I/usr/local/include/imnodes
LDFLAGS = -L/usr/local/lib
LIBS = -limgui -limnodes -lSDL2 -lglfw -lGL
OBJS = main.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f imgui.ini
