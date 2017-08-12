CFLAGS  += -Wall -Wextra -pedantic -Wno-format -std=c99

all: createGCT createGPF generateBl

createGCT: createGCT.c
createGPF: createGPF.c
generateBl: generateBl.c

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

.PHONY: all clean
