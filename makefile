cube:
	$(CC) cube.c `pkg-config --cflags --libs sdl3` -lm -g -Wall
camera:
	$(CC) camera.c `pkg-config --cflags --libs sdl3` -lm -g -Wall
camera_face:
	$(CC) camera_face.c `pkg-config --cflags --libs sdl3` -lm -g -Wall -O0
