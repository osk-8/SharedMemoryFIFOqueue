TARGET = test_producent
LIBS = -lrt -pthread
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

run: $(TARGET)
	@./$(TARGET)

crun: $(TARGET)
	@clear
	@./$(TARGET)

valgrind: $(TARGET)
	@valgrind --leak-check=full ./$(TARGET)

gdb: $(TARGET)
	@gdb ./$(TARGET)

clean:
	-rm -f *.o
	-rm -f $(TARGET)