# https://github.com/raysan5/raylib/wiki/Working-on-macOS
eval cc src/main.c src/player.c src/level.c src/boids.c -framework IOKit -framework Cocoa -framework OpenGL $(pkg-config --libs --cflags raylib) -o game.exe

# Exit the script if the last command was unsuccessful
if [ $? -ne 0 ]; then
	exit 1
fi

# Run the executable that was created
./game.exe
