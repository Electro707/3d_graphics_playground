// ChatGPT
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define APP_W (240)
#define APP_H (240)

typedef uint32_t u32;
typedef int16_t q15;

#define CUBE_MATRIX_ROWS	10

typedef struct{
	u32 rows;
	u32 cols;
} matrixSize_s;

void drawLine(u32 x0, u32 y0, u32 x1, u32 y1, u32 color);
void matrixMult(q15 *srcA, q15 *srcB, q15 *dst, matrixSize_s sizeA, matrixSize_s sizeB);

static uint32_t fb[APP_W * APP_H]; // 32-bit ARGB8888

/* defines for object */
q15 objectVertex[32*4] = {
        31130, 15565, -11674, 31130,
        31130, -15565, -11674, 31130,
        31130, 15565, 11674, 31130,
        31130, -15565, 11674, 31130,
        -31130, 15565, -11674, 31130,
        -31130, -15565, -11674, 31130,
        -31130, 15565, 11674, 31130,
        -31130, -15565, 11674, 31130,
        0, 11674, -23347, 31130,
        0, 11674, -11674, 31130,
        8254, 8254, -23347, 31130,
        8254, 8254, -11674, 31130,
        11674, 0, -23347, 31130,
        11674, 0, -11674, 31130,
        8254, -8254, -23347, 31130,
        8254, -8254, -11674, 31130,
        0, -11674, -23347, 31130,
        0, -11674, -11674, 31130,
        -8254, -8254, -23347, 31130,
        -8254, -8254, -11674, 31130,
        -11674, 0, -23347, 31130,
        -11674, 0, -11674, 31130,
        -8254, 8254, -23347, 31130,
        -8254, 8254, -11674, 31130,
        -24904, 15565, 4669, 31130,
        -24904, 20234, 4669, 31130,
        -24904, 15565, -4669, 31130,
        -24904, 20234, -4669, 31130,
        -15565, 15565, 4669, 31130,
        -15565, 20234, 4669, 31130,
        -15565, 15565, -4669, 31130,
        -15565, 20234, -4669, 31130,
};
uint8_t objectEdge[48*2] =     {
        0, 4,   4, 6,   6, 2,   2, 0,
        3, 2,   6, 7,   7, 3,   4, 5,
        5, 7,   5, 1,   1, 3,   1, 0,
        8, 9,   9, 11,  11, 10, 10, 8,
        11, 13, 13, 12, 12, 10, 13, 15,
        15, 14, 14, 12, 15, 17, 17, 16,
        16, 14, 17, 19, 19, 18, 18, 16,
        19, 21, 21, 20, 20, 18, 9, 23,
        23, 21, 23, 22, 22, 20, 8, 22,
        24, 25, 25, 27, 27, 26, 26, 24,
        27, 31, 31, 30, 30, 26, 31, 29,
        29, 28, 28, 30, 29, 25, 24, 28,
};

static inline void putPixel(int x, int y, uint32_t c) {
	if ((unsigned)x < APP_W && (unsigned)y < APP_H) {
		fb[(y * APP_W) + x] = c;
	}
}


q15 sat16(int32_t x){
	if (x > 0x7FFF) return 0x7FFF;
	else if (x < -0x8000) return -0x8000;
	else return (q15)x;
}

q15 q15Mult(q15 a, q15 b){
	int32_t res = (int32_t)a * (int32_t)b;
	res += 16384;
	return sat16(res >> 15);
}

u32 scaleVectorToDraw(q15 pos){
	// return (u32)((pos >> 9) + (APP_W >> 1));
	return (u32)(q15Mult(pos, 100.) + (APP_W >> 1));
}

// todo: this is converting with floats for now, todo lookup table
// rad is from 0 to 1.0 in q15 format, which is converted to 0 to 2*pi
q15 cosFP(q15 rad){
	float tmp;
	
	tmp = rad;
	tmp /= 32768.0;
	tmp *= M_PI * 2;

	tmp = cos(tmp);

	tmp *= 32768.0;
	return sat16(tmp);
}
q15 sinFP(q15 rad){
	float tmp;
	
	tmp = rad;
	tmp /= 32767.0;
	tmp *= M_PI * 2;

	tmp = sin(tmp);

	tmp *= 32767.0;
	return (q15)tmp;
}

void createTransformMatrix(q15 rotX, q15 rotY, q15 rotZ, q15 *resultMatrix){
	q15 tmpA[4*4];
	q15 tmpB[4*4];

	q15 rotationMatrixX[4*4] = {
		32767, 0,         0,          0,
		0, cosFP(rotX), -sinFP(rotX), 0,
		0, sinFP(rotX), cosFP(rotX),  0,
		0, 0,         0,          32767,
	};
	q15 rotationMatrixY[4*4] = {
		cosFP(rotY),  0, sinFP(rotY), 0,
		0,          32767, 0,         0,
		-sinFP(rotY), 0, cosFP(rotY), 0,
		0,          0, 0,         32767,
	};
	q15 rotationMatrixZ[4*4] = {
		cosFP(rotZ), -sinFP(rotZ), 0, 0,
		sinFP(rotZ), cosFP(rotZ),  0, 0,
		0,         0,          32767, 0,
		0,         0,          0, 32767,
	};

	q15 inverseMatrix[4*4] = {
		32767, 0, 0, 0,
		0, -32768,  0, 0,
		0,     0,          32767, 0,
		0,         0,          0, 32767,
	};

	matrixMult(rotationMatrixX, rotationMatrixY, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixY, tmpB, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpB, rotationMatrixZ, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, inverseMatrix, resultMatrix, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
}

void matrixMult(q15 *srcA, q15 *srcB, q15 *dst, matrixSize_s sizeA, matrixSize_s sizeB){
	int32_t sum;

	assert(sizeA.cols == sizeB.rows);

	u32 m = sizeA.rows;
	u32 p = sizeB.cols;
	u32 n = sizeA.cols;

	memset(dst, 0, m * p * sizeof(q15));

	for(int i = 0; i < m; i++){
		for(int j = 0; j < p; j++){
			sum = 0;
			for(int k = 0; k < n; k++){
				sum += q15Mult(srcA[k+(i*sizeA.cols)], srcB[j+(k*sizeB.cols)]);
			}
			dst[(i*sizeB.cols) + j] = sat16(sum);
		}
	}
}

void drawObj(q15 *obj, uint8_t *faceDef, uint objEdgeLen){
	q15 *pos;
	q15 *posNext;

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



	q15 newPos[32*4];
	q15 txMatrix[4*4];
	// matrixMult((u32 *)cubePositions, (u32 *)perspectiveMatrix, (u32 *)newPos, (matrixSize_s){ 4, 8}, (matrixSize_s){4, 4});


	// Simple event loop to keep window open
	SDL_Event e;
	int quit = 0;
	q15 rot = 0;
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT)
				quit = 1;
		}

		SDL_memset(fb, 0, APP_W * APP_H * sizeof(uint32_t));

		createTransformMatrix(-1820, rot, 0, txMatrix);
		matrixMult(objectVertex, txMatrix, newPos, (matrixSize_s){32, 4}, (matrixSize_s){4, 4});

		// creteAndRotateObj(objectVertex, 32, 20., rot, 0, newPos);
		drawObj(newPos, objectEdge, 48);

		SDL_memcpy(surf->pixels, fb, APP_W * APP_H * sizeof(uint32_t));
		SDL_UpdateWindowSurface(win);

		rot += 91;
		if(rot > 0x7FFF){
			rot = 0;
		}

		SDL_Delay(50);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}