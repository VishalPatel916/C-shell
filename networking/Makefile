# Compiler and flags
CC = gcc
# Use -g for debugging, -O2 for optimization. -Wall shows all warnings.
CFLAGS = -g -Wall
LDFLAGS = -lcrypto
RM = rm -f

# List of final executables
TARGETS = server client 

# Auto-detect the OS for platform-specific flags
OS := $(shell uname -s)
ifeq ($(OS), Darwin) # Darwin is the kernel name for macOS
    # macOS specific flags for Homebrew's OpenSSL
    BREW_PREFIX := $(shell brew --prefix openssl)
    CFLAGS += -I$(BREW_PREFIX)/include
    LDFLAGS += -L$(BREW_PREFIX)/lib
endif

# Default rule: builds all targets
all: $(TARGETS)

# Rule to link the server executable
server: server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to link the client executable
client: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule to compile .c files into .o (object) files
# This rule also ensures that if sham.h is modified, the code is recompiled.
%.o: %.c sham.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up generated files
clean:
	$(RM) $(TARGETS) *.o

# Declare phony targets that don't represent actual files
.PHONY: all clean