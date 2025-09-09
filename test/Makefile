CC = gcc

CFLAGS = -g -Wall

LDFLAGS = -I../

TARGET = test_vlog

SRCS = ../vlog.c test_vlog.c

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
    strftime \
    fputs 

WRAPFLAGS = $(foreach func,$(WRAP_FUNCS),-Wl,--wrap=$(func))

all: $(TARGET)

$(TARGET):
	$(CC) $(SRCS) $(CFLAGS) -o $@ $(LDFLAGS) $(WRAPFLAGS) $(LIBS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean run