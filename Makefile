make:
	gcc -g main.c stretchy_buffer.c -lm -lSDL2 -lSDL2_ttf -lSDL2_mixer -o game

windows:
	gcc -g main.c stretchy_buffer.c -lm -lSDL2 -lSDL2_ttf -lSDL2_mixer -o game \
		-I"G:\.minlib\SDL2-2.0.7\x86_64-w64-mingw32\include" \
		-I"G:\.minlib\SDL2_ttf-2.0.14\x86_64-w64-mingw32\include" \
		-I"G:\.minlib\SDL2_mixer-2.0.2\x86_64-w64-mingw32\include" \
		-L"G:\.minlib\SDL2-2.0.7\x86_64-w64-mingw32\lib" \
		-L"G:\.minlib\SDL2_ttf-2.0.14\x86_64-w64-mingw32\lib" \
		-L"G:\.minlib\SDL2_mixer-2.0.2\x86_64-w64-mingw32\lib"
