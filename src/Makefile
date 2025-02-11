# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell python3-config --cflags)
LDFLAGS = $(shell python3-config --ldflags)

# Target executable name
TARGET = main

# Default rule: compile the program
all: $(TARGET)

# Rule to compile main.c into the TARGET executable
$(TARGET): seq2torch.c
	$(CC) $(CFLAGS) -o $(TARGET) seq2torch.c $(LDFLAGS)

# 'run' rule: compile (if needed) and then run the executable
run: all
	./$(TARGET)

# Clean rule: remove the compiled executable
clean:
	rm -f $(TARGET)

# Declare non-file targets as phony
.PHONY: all run clean


#make run to run and compile, make all to just compile, make clean to clean