TARGET=emtv

VPATH=../src ../include

CFLAGS=-I../include -I/opt/X11/include -g
# LDFLAGS=-Wl,-rpath,/usr/local/lib
LDFLAGS=-L/opt/X11/lib
# LDLIBS=-lglut -lGL -ldl
LDLIBS=-framework OpenGL -lGLUT

.SUFFIXES: .c .o

SRCS=$(notdir $(wildcard ../src/*.c))
OBJS=$(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
