# Source emsdk for this terminal session if last command failed
if [ -z "$EMSDK" ]; then
	source $HOME/emsdk/emsdk_env.sh
fi

# Try make web directory if doesn't exist
mkdir -p web

# https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5)

# NOTE:
# Can remove -s ASYNCIFY if there is no while(!WindowShouldClose()) loop
# --preload-file src/resources must be added if you want to load assets
emcc -o web/game.html src/main.c src/player.c src/level.c, src/boids.c -Os -Wall $HOME/raylib/src/web/libraylib.web.a \
	-I. -I$HOME/raylib/src -L. -L$HOME/raylib/src/web \
	-s USE_GLFW=3 \
	-s ASYNCIFY \
	--shell-file $HOME/raylib/src/minshell.html \
	--preload-file src/resources \
	-s TOTAL_STACK=64MB \
	-s INITIAL_MEMORY=128MB \
	-DPLATFORM_WEB

# Run the web build if it build successfully
if [ $? -eq 0 ]; then
	emrun web/game.html
fi
