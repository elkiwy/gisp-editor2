SRC := src
OBJ := obj
BUILD := build

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

FLAGS = -g -Og -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -fsanitize=address -fsanitize=undefined
LIBS = -L /usr/local/lib -l SDL2-2.0.0 -l SDL2_ttf



build/gisp-editor2: clean $(OBJECTS)
	gcc $(OBJECTS) $(FLAGS) $(LIBS) -o $@


$(OBJ)/%.o: $(SRC)/%.c
	gcc $(FLAGS) -c $< -o $@

clean:
	rm -f $(BUILD)/gisp-editor2 && rm -f $(OBJ)/*.o

run: build/gisp-editor2
	./build/gisp-editor2


test: build/gisp-editor2
	./build/gisp-editor2 test.txt
