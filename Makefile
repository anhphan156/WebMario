WIDTH := 1920
HEIGHT := 1080

CC := gcc
LDFLAGS := -lm -lraylib
CFLAGS := -Wall -g -DWIDTH=$(WIDTH) -DHEIGHT=$(HEIGHT)

SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%.c=%.o)
EXECUTABLE = out

.PHONY: all clean web test

all: $(EXECUTABLE)

web: CC:=emcc
web: LDFLAGS := -0 -lm ./dependencies/lib/libraylib.a -s FORCE_FILESYSTEM=1 -s USE_GLFW=3 --shell-file ./minshell.html --preload-file ./gamedata
web: CFLAGS += -DPLATFORM_WEB
web: $(OBJS)
	@mkdir -p game
	$(CC) -o game/index.html $(addprefix $(BUILD_DIR)/,$(notdir $^)) $(LDFLAGS)

test: CFLAGS := -DTEST
test: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) -o $@ $(addprefix $(BUILD_DIR)/,$(notdir $^)) $(LDFLAGS)

$(OBJS): %.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $(addprefix $(BUILD_DIR)/,$(notdir $@)) -I$(INCLUDE_DIR)

run:
	./out

debug:
	gdb ./out

leak-check:
	#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=leak.txt ./build/app
	valgrind --leak-check=full --log-file=leak.txt ./out

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE) html/

raylib:
	emcc -o build/rcore.o -c ./dependencies/raylib/src/rcore.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	emcc -o build/rshapes.o -c ./dependencies/raylib/src/rshapes.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	emcc -o build/rtextures.o -c ./dependencies/raylib/src/rtextures.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	emcc -o build/rtext.o -c ./dependencies/raylib/src/rtext.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	emcc -o build/rmodels.o -c ./dependencies/raylib/src/rmodels.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	emcc -o build/utils.o -c ./dependencies/raylib/src/utils.c -Os -Wall -DPLATFORM_WEB
	emcc -o build/raudio.o -c ./dependencies/raylib/src/raudio.c -Os -Wall -DPLATFORM_WEB

	emar rcs dependencies/raylib/lib/libraylib.a build/rcore.o build/rshapes.o build/rtextures.o build/rtext.o build/rmodels.o build/utils.o build/raudio.o
