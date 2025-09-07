// partial ChatGPT (only for sdl3 setup), rest is by a human (me!)
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "objCamera.h"

#define APP_W (240)
#define APP_H (240)

typedef uint32_t u32;

#define KEYBOARD_ROTATION
#define ROTATION_INCREMENT	5

typedef struct{
	u32 rows;
	u32 cols;
} matrixSize_s;

void drawLine(int x0, int y0, int x1, int y1, u32 color);
void matrixMult(float *srcA, float *srcB, float *dst, matrixSize_s sizeA, matrixSize_s sizeB);

static uint32_t fb[APP_W * APP_H]; // 32-bit ARGB8888

static inline void putPixel(int x, int y, uint32_t c) {
	if ((unsigned)x < APP_W && (unsigned)y < APP_H) {
		fb[y * APP_W + x] = c;
	}
}
// float perspectiveMatrix[4*4] = {
// 	1, 0, 0, 0,
// 	0, 1, 0, 0,
// 	0, 0, 1, 0,
// 	0, 0, 0, 1,
// };

u32 scaleVectorToDraw(float pos){
	return (u32)((pos * 40) + (APP_W/2));
}

void creteAndRotateObj(float *obj, uint objVertexLen, float rotX, float rotY, float rotZ, float *newObj){
	float tmpA[4*objVertexLen];
	float tmpB[4*objVertexLen];

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

	matrixMult(rotationMatrixX, rotationMatrixY, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixY, tmpB, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpB, rotationMatrixZ, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});

	matrixMult(obj, tmpA, newObj, (matrixSize_s){objVertexLen, 4}, (matrixSize_s){4, 4});
}

void createTransformMatrix(float rotX, float rotY, float rotZ, float *resultMatrix){
	float tmpA[4*4];
	float tmpB[4*4];

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

	// float inverseMatrix[4*4] = {
	// 	1, 0, 0, 0,
	// 	0, 1, 0, 0,
	// 	0, 0, 1, 0,
	// 	0, 0, 0, 1,
	// };

	float toViewCoordsMatrixTranslate[4*4] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 2,
	0, 0, 0, 1,
	}; 

	float perspectiveMatrix[4*4] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 1, 0,
	};

	matrixMult(perspectiveMatrix, toViewCoordsMatrixTranslate, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixX, tmpB, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpB, rotationMatrixY, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixZ, resultMatrix, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});

	// matrixMult(tmpA, perspectiveMatrix, resultMatrix, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
}

void homegenousDivide(float *matrix, uint colLen){
	float wc;
	for(int c=0;c<colLen;c++){
		wc = matrix[(c*4)+3];
		for(int i=0;i<4;i++){
			matrix[(c*4)+i] /= wc;
		}
	}
}

void transposeMatrix(float *matrix, matrixSize_s size){
	float newMatrix[size.cols * size.rows];

	assert(size.cols == size.rows);

	for(int row=0;row<size.cols;row++){
		for(int col=0;col<size.cols;col++){
			newMatrix[size.cols*row + col] = matrix[size.cols*col + row];
		}
	}

	for(int i=0;i<size.cols*size.rows;i++){
		matrix[i] = newMatrix[i];
	}
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

void drawObj(float *obj, uint *faceDef, uint objEdgeLen){
	float *pos;
	float *posNext;

	for(int i=0;i<objEdgeLen;i++){
		pos = &obj[4*(faceDef[i*2])];
		posNext = &obj[4*(faceDef[(i*2)+1])];

		drawLine(scaleVectorToDraw(pos[0]),
				scaleVectorToDraw(pos[1]),
				scaleVectorToDraw(posNext[0]),
				scaleVectorToDraw(posNext[1]),
				0x00FFFFFF);
	}

	// for(int i=0;i<objEdgeLen;i++){
	// 	putPixel(scaleVectorToDraw(obj[4*i]), scaleVectorToDraw(obj[(4*i)+1]), 0x00FFFFFF);
	// }
	
}

void drawLine(int x0, int y0, int x1, int y1, u32 color){
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
	if (SDL_Init(SDL_INIT_VIDEO) == false) {
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



	float newPos[VERTEX_SIZE*4];
	float txMatrix[4*4];
	// matrixMult((u32 *)cubePositions, (u32 *)perspectiveMatrix, (u32 *)newPos, (matrixSize_s){ 4, 8}, (matrixSize_s){4, 4});


	// Simple event loop to keep window open
	SDL_Event e;
	int quit = 0;
	float rotX = 180.0;
	float rotY = 0.0;
	float rotZ = 0.0;

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT){
				quit = 1;
			}
#ifdef KEYBOARD_ROTATION
			else if(e.type == SDL_EVENT_KEY_DOWN){
				switch(e.key.key){
					case SDLK_DOWN:
						rotX -= ROTATION_INCREMENT;
						break;
					case SDLK_UP:
						rotX += ROTATION_INCREMENT;
						break;
					case SDLK_LEFT:
						rotY -= ROTATION_INCREMENT;
						break;
					case SDLK_RIGHT:
						rotY += ROTATION_INCREMENT;
						break;
				}
				printf("%f - %f\n", rotX, rotY);
			}
#endif
		}

		SDL_memset(fb, 0, APP_W * APP_H * sizeof(uint32_t));

		createTransformMatrix(rotX, rotY, rotZ, txMatrix);
		transposeMatrix(txMatrix, (matrixSize_s){4, 4});
		matrixMult(objectVertex, txMatrix, newPos, (matrixSize_s){VERTEX_SIZE, 4}, (matrixSize_s){4, 4});

		homegenousDivide(newPos, VERTEX_SIZE);

		// creteAndRotateObj(objectVertex, 32, 20., rot, 0, newPos);
		drawObj(newPos, objectEdge, EDGE_SIZE);

		SDL_memcpy(surf->pixels, fb, APP_W * APP_H * sizeof(uint32_t));
		SDL_UpdateWindowSurface(win);

#ifndef KEYBOARD_ROTATION
		rotY += 1;
		if(rotY > 360){
			rotY = 0;
		}
#endif

		SDL_Delay(50);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}