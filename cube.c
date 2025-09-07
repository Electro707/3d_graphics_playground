// ChatGPT
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define APP_W 240
#define APP_H 240

typedef uint32_t u32;

#define CUBE_MATRIX_ROWS	10

typedef struct{
	u32 rows;
	u32 cols;
} matrixSize_s;

void drawLine(u32 x0, u32 y0, u32 x1, u32 y1, u32 color);
void matrixMult(float *srcA, float *srcB, float *dst, matrixSize_s sizeA, matrixSize_s sizeB);

static uint32_t fb[APP_W * APP_H]; // 32-bit ARGB8888

static inline void putPixel(int x, int y, uint32_t c) {
	if ((unsigned)x < APP_W && (unsigned)y < APP_H) {
		fb[y * APP_W + x] = c;
	}
}

float cubePositions[CUBE_MATRIX_ROWS*4] = {
	-1, 1, -1, 		1,
	1, 1, -1, 		1,
	1, -1, -1, 		1,
	-1, -1, -1, 	1,
	-1, 1, -1, 		1,

	-1, 1, 1, 		1,
	1, 1, 1, 		1,
	1, -1, 1, 		1,
	-1, -1, 1, 		1,
	-1, 1, 1, 		1,
};

float perspectiveMatrix[4*4] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

u32 scaleVectorToDraw(float pos){
	return (pos * 50) + 120;
}

void rotateCube(float rotX, float rotY, float rotZ, float *newCube){
	float tmpA[4*CUBE_MATRIX_ROWS];
	float tmpB[4*CUBE_MATRIX_ROWS];

	rotX = rotX * (M_PI/180);
	rotY = rotY * (M_PI/180);
	rotZ = rotZ * (M_PI/180);

	float rotationMatrixX[4*4] = {
		1, 0,         0,          0,
		0, cos(rotX), -sin(rotX), 0,
		0, sin(rotX), cos(rotX),  0,
		0, 0,         0,          1,
	};
	float rotationMatrixY[4*4] = {
		cos(rotY),  0, sin(rotY), 0,
		0,          1, 0,         0,
		-sin(rotY), 0, cos(rotY), 0,
		0,          0, 0,         1,
	};
	float rotationMatrixZ[4*4] = {
		cos(rotZ), -sin(rotZ), 0, 0,
		sin(rotZ), cos(rotZ),  0, 0,
		0,         0,          1, 0,
		0,         0,          0, 1,
	};

	matrixMult(cubePositions, rotationMatrixX, tmpA, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixY, tmpB, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpB, rotationMatrixZ, newCube, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
}

void matrixMult(float *srcA, float *srcB, float *dst, matrixSize_s sizeA, matrixSize_s sizeB){
	float sum;

	assert(sizeA.cols == sizeB.rows);

	u32 m = sizeA.rows;
	u32 p = sizeB.cols;
	u32 n = sizeA.cols;

	memset(dst, 0, m * p * sizeof(float));

	for(int i = 0; i < m; i++){
		for(int j = 0; j < p; j++){
			sum = 0;
			for(int k = 0; k < n; k++){
				sum += srcA[k+(i*sizeA.cols)] * srcB[j+(k*sizeB.cols)];
			}
			dst[(i*sizeB.cols) + j] = sum;
		}
	}
}

void drawCube(float *cube){
	float *pos;
	float *posNext;
	// for(int i=0;i<(CUBE_MATRIX_ROWS-1);i++){
	// 	pos = &cube[4*i];
	// 	posNext = &cube[4*(i+1)];
 //
	// 	drawLine(scaleVectorToDraw(pos[0]),
	// 			 scaleVectorToDraw(pos[1]),
	// 			 scaleVectorToDraw(posNext[0]),
	// 			 scaleVectorToDraw(posNext[1]),
	// 			 0x00FFFFFF);
	// }

	for(int side = 0; side < 2; side++){
		for(int i=0;i<4;i++){
			pos = &cube[(4*i) + (side*5*4)];
			posNext = &cube[4*(i+1) + (side*5*4)];

			drawLine(scaleVectorToDraw(pos[0]),
					scaleVectorToDraw(pos[1]),
					scaleVectorToDraw(posNext[0]),
					scaleVectorToDraw(posNext[1]),
					0x00FFFFFF);

		}
	}

	for(int p = 0; p < 4; p++){
		pos = &cube[4*p];
		posNext = &cube[4*(p+5)];

		drawLine(scaleVectorToDraw(pos[0]),
				scaleVectorToDraw(pos[1]),
				scaleVectorToDraw(posNext[0]),
				scaleVectorToDraw(posNext[1]),
				0x00FFFFFF);
	}
}

void drawLine(u32 x0, u32 y0, u32 x1, u32 y1, u32 color){
	// https://en.wikipedia.org/wiki/Bresenham's_line_algorithm#All_cases
	int16_t dx = abs(x1-x0);
	int16_t dy = -abs(y1-y0);
	int16_t sx = x0 < x1 ? 1 : -1;
	int16_t sy = y0 < y1 ? 1 : -1;
	int16_t error = dx + dy;

	 while(1){
		putPixel(x0, y0, color);

		if((2*error) >= dy){
			if(x0 == x1){break;}
			error += dy;
			x0 += sx;
		}
		if((2*error) <= dx){
			if(y0 == y1){break;}
			error += dx;
			y0 += sy;
		}
	}
}


int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("SDL init failed: %s", SDL_GetError());
		return 1;
	}

	SDL_Window *win = SDL_CreateWindow("fb", APP_W, APP_H, 0);
	if (!win) {
		SDL_Log("CreateWindow failed: %s", SDL_GetError());
		return 1;
	}

	SDL_Surface *surf = SDL_GetWindowSurface(win);

	// drawLine(0, 0, 100, 150, 0x00FFFFFF);



	float newPos[CUBE_MATRIX_ROWS*4];
	// matrixMult((u32 *)cubePositions, (u32 *)perspectiveMatrix, (u32 *)newPos, (matrixSize_s){ 4, 8}, (matrixSize_s){4, 4});


	// Simple event loop to keep window open
	SDL_Event e;
	int quit = 0;
	float rot = 0.0;
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT)
				quit = 1;
		}

		SDL_memset(fb, 0, APP_W * APP_H * sizeof(uint32_t));

		rotateCube(rot, rot*2, rot, newPos);
		drawCube(newPos);

		// Copy fb[] into window surface
		SDL_memcpy(surf->pixels, fb, APP_W * APP_H * sizeof(uint32_t));
		SDL_UpdateWindowSurface(win);

		rot += 1;
		if(rot > 360){
			rot = 0;
		}

		SDL_Delay(50);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
