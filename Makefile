CC = gcc

CFLAGS = -g -Wall

LDFLAGS =

TARGET = test_vlog

SRCS = vlog.c test_vlog.c
OBJS = $(SRCS:.c=.o)

LIBS = -lcmocka -lpthread

WRAP_FUNCS = \
    fopen \
    fclose \
    ftell \
    fseek \
    rewind \
    fileno \
    ftruncate \
    fflush \
    vfprintf \
    fputc \
    fprintf \
    strftime \
    fputs 

WRAPFLAGS = $(foreach func,$(WRAP_FUNCS),-Wl,--wrap=$(func))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(WRAPFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean run