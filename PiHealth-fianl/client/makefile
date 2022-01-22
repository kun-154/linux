src = $(wildcard ./src/*.c)
obj = $(patsubst ./src/%.c, ./obj/%.o, $(src))

CC = gcc
CFLAGS = -lpthread -g
inc_path = ./inc

ALL:a.out

a.out: $(obj)
	$(CC) $^ -o $@ $(CFLAGS)

$(obj):./obj/%.o:./src/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -I $(inc_path)  

clean:
	-rm -rf $(obj) a.out

.PHONY: clean ALL
