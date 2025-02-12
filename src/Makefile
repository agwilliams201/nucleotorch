# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell python3-config --cflags) -I$(shell python3 -c "import numpy; print(numpy.get_include())")

# Use -shared, -fPIC and -undefined dynamic_lookup for a Python extension module.
LDFLAGS = $(shell python3-config --ldflags) -shared -fPIC -undefined dynamic_lookup

# Target shared library name (note the .so extension for a Python extension)
TARGET = nucleotorch.so

# Default rule: compile the module
all: $(TARGET)

$(TARGET): seq2torch.c
	$(CC) $(CFLAGS) -o $(TARGET) seq2torch.c $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean