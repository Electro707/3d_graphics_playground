// ChatGPT
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "objCamera.h"
// #include "objCube.h"

#define APP_W (240*2)
#define APP_H (240*2)

typedef uint32_t u32;

#define CUBE_VERTEX	8
#define CUBE_FACE			6
#define CUBE_EDGE			12

#define FACE_VERTEX_SIZE	3

typedef struct{
	u32 rows;
	u32 cols;
} matrixSize_s;

void drawLine(u32 x0, u32 y0, u32 x1, u32 y1, u32 color);
void matrixMult(float *srcA, float *srcB, float *dst, matrixSize_s sizeA, matrixSize_s sizeB);

static u32 colorPerOjb[] = {
	0xFF0000,
	0x00FF00,
	0x0000FF,
	0xFFFF00,
	0x00FFFF,
	0xFF00FF,
	0xFFFFFF
};

static uint32_t fb[APP_W * APP_H]; // 32-bit ARGB8888

static inline void putPixel(int x, int y, uint32_t c) {
	if ((unsigned)x < APP_W && (unsigned)y < APP_H) {
		fb[y * APP_W + x] = c;
	}
}

float cubePositions[CUBE_VERTEX*4] = {
	-1, 1, -1, 		1,
	1, 1, -1, 		1,
	1, -1, -1, 		1,
	-1, -1, -1, 	1,
	-1, 1, 1, 		1,
	1, 1, 1, 		1,
	1, -1, 1, 		1,
	-1, -1, 1, 		1,
};

u32 cubeEdges[CUBE_EDGE*2] = {
	0, 1,	1, 2,	2, 3, 	3, 0,
	4, 5,	5, 6,	6, 7,	7, 4,
	0, 4,	1, 5,	2, 6,	3, 7,
};

u32 cubeFace[CUBE_FACE*4] = {
	0, 1, 2, 3,
	4, 5, 6, 7,
	0, 4, 5, 1,
	1, 5, 6, 2,
	2, 6, 7, 3,
	3, 7, 4, 0,
};

float perspectiveMatrix[4*4] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

float toViewCoordsMatrixScale[4*4] = {
	80, 0, 0, 0,
	0, 80, 0, 0,
	0, 0, 80, 0,
	0, 0, 0, 1,
}; 

float toViewCoordsMatrixTranslate[4*4] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	((float)APP_W/2), ((float)APP_H/2), 0, 1,
}; 

int16_t scaleVectorToDraw(float pos){
	return (int16_t)pos;
	// return (pos * 80) + (APP_W/2);
}
float unscaleVectorToDraw(int16_t pos){
	// return ((float)pos - (float)(APP_W/2))/80.;
	return (float)pos;
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

	float inverseMatrix[4*4] = {
		1, 0, 0, 0,
		0, -1,  0, 0,
		0,     0,          1, 0,
		0,         0,          0, 1,
	};

	matrixMult(rotationMatrixX, rotationMatrixY, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, rotationMatrixY, tmpB, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpB, rotationMatrixZ, tmpA, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
	matrixMult(tmpA, inverseMatrix, resultMatrix, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
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

int getPlaneZ(float x, float y, int *planeNormal, int *planeVertex){
	if(planeNormal[2] == 0){
		return -1000;
	}

	int d = (planeNormal[0]*planeVertex[0]) + (planeNormal[1]*planeVertex[1]) + (planeNormal[2]*planeVertex[2]);
	int z = (x*planeNormal[0])+(y*planeNormal[1]) - d;
	z /= -planeNormal[2];
	return z;
}

void drawObj(int *obj, uint *edgeToVertex, uint edgeToVertexLen, uint *faceDef, uint faceDefLen, int *faceNormals, uint *faceToNormalIdx){
	int pos[2];
	int posNext[2];
	u32 *face;
	int *posFP;
	int *posFPP;
	u32 color = 0xFFFFFF;
	int tmpF;

// #define DRAW_FACE
	// face drawing function
#ifdef DRAW_FACE
	for(int fI=0;fI<faceDefLen;fI++){
		face = &faceDef[fI*FACE_VERTEX_SIZE];

		// color = 0x999999;
		// for(int i=0;i<OBJ_TYPES_N;i++){
		// 	if(fI < objectTypeSplitFace[i]){
		// 		color = colorPerOjb[i];
		// 		break;
		// 	}
		// }
		color = colorPerOjb[fI % 7];

		if(face[0] == 255){
			continue;
		}
		if(faceNormals[(faceToNormalIdx[fI]*4) + 2] < 0.0){
			// printf("%f awan\n", faceNormals[(faceToNormalIdx[fI]*4) + 2] );
			continue;
		}

		u32 maxX = 0;
		u32 maxY = 0;
		u32 minX = 1000;
		u32 minY = 1000;

		float tmp;

		for(int i=0;i<FACE_VERTEX_SIZE;i++){
			for(int ii=0;ii<4;ii++)
				pos[ii] = scaleVectorToDraw(obj[(4*face[i])+ii]);
			if(pos[0] > maxX)
				maxX = pos[0];
			if(pos[1] > maxY)
				maxY = pos[1];
			if(pos[0] < minX)
				minX = pos[0];
			if(pos[1] < minY)
				minY = pos[1];
		}

		for(int y=minY;y<=maxY;y++){
			for(int x=minX;x<=maxX;x++){

				bool isInside = true;
				for(int i=0;i<FACE_VERTEX_SIZE;i++){
					for(int ii=0;ii<4;ii++)
						pos[ii] = scaleVectorToDraw( obj[(4*face[i%FACE_VERTEX_SIZE])+ii] );
					for(int ii=0;ii<4;ii++)
						posNext[ii] = scaleVectorToDraw( obj[(4*face[(i+1)%FACE_VERTEX_SIZE])+ii] );

					tmp = (((float)x-(float)pos[0])*((float)posNext[1]-(float)pos[1]) ) - (((float)y-(float)pos[1])*(float)((float)posNext[0]-(float)pos[0]));
					if(tmp < 0){
						isInside = false;
						break;
					}
				}

				if(isInside){
					putPixel(x, y, color);
				}
			}
		}
	}
#endif

	// line function
	uint vertex1, vertex2;
	bool toDraw;
	
	for(uint edgeI=0;edgeI<edgeToVertexLen;edgeI++){
		vertex1 = edgeToVertex[edgeI*2];
		vertex2 = edgeToVertex[(edgeI*2)+1];

		posFP = &obj[4*vertex1];
		posFPP = &obj[4*vertex2];

		// reject edge if two faces connected to it have same normal
		// note: commented out as this was done by the Python script already
		/*
		int lastFaceId = -1;
		bool normalEqual = false;
		for(int fId=0;fId<faceDefLen;fId++){
			face = &faceDef[fId*FACE_VERTEX_SIZE];
			uint isVertexInFace = 0;
			for(int i=0;i<FACE_VERTEX_SIZE;i++){
				if(face[i] == vertex1 || face[i] == vertex2){
					isVertexInFace += 1;
				}
			}
			if(isVertexInFace >= 2){
				if(lastFaceId == -1){
					lastFaceId = fId;
				}
				else {
					float *normA = &faceNormals[4*faceToNormalIdx[fId]];
					float *normB = &faceNormals[4*faceToNormalIdx[lastFaceId]];
					lastFaceId = -1;
					normalEqual = true;
					for(int i=0;i<4;i++){
						if(normA != normB){
							normalEqual = false;
							break;
						}
					}
					if(normalEqual){
						break;
					}
				}
			}
		}
		if(normalEqual){
			continue;
		}
		*/


		int x0 = posFP[0];
		int y0 = posFP[1];
		int z0 = posFP[2];
		int x1 = posFPP[0];
		int y1 = posFPP[1];
		int z1 = posFPP[2];
		
		int dx = abs(x1-x0);
		int dy = abs(y1-y0);
		int dz = abs(z1-z0);
		int sx = x0 < x1 ? 1 : -1;
		int sy = y0 < y1 ? 1 : -1;
		int sz = z0 < z1 ? 1 : -1;

		int dm = dx;
		if(dy > dm)
			dm = dy;
		if(dz > dm)
			dm = dz;
		
		int i = dm;

		// https://zingl.github.io/Bresenham.pdf

		int z0P;

		int edgeFunc[FACE_VERTEX_SIZE*faceDefLen];
		int edgeDx[FACE_VERTEX_SIZE*faceDefLen];
		int edgeDy[FACE_VERTEX_SIZE*faceDefLen];

		for(uint fI=0;fI<faceDefLen;fI++){
			face = &faceDef[fI*FACE_VERTEX_SIZE];
			for(int i=0;i<FACE_VERTEX_SIZE;i++){
				for(int ii=0;ii<2;ii++)
					pos[ii] = obj[(4*face[i%FACE_VERTEX_SIZE])+ii];
				for(int ii=0;ii<2;ii++)
					posNext[ii] = obj[(4*face[(i+1)%FACE_VERTEX_SIZE])+ii];

				edgeDx[fI*FACE_VERTEX_SIZE + i] = posNext[0] - pos[0];
				edgeDy[fI*FACE_VERTEX_SIZE + i] = posNext[1] - pos[1];
				edgeFunc[fI*FACE_VERTEX_SIZE + i] = ((x0-pos[0])*edgeDy[fI*FACE_VERTEX_SIZE + i]) - ((y0-pos[1])*edgeDx[fI*FACE_VERTEX_SIZE + i]);
			}
		}
		
		for(x1=y1=z1= i/2; i-- >= 0; ){
			
			if(edgeI == 2){
				color = 0xFF0000;
			} else {
				color = 0xFFFFFF;
			}


			toDraw = true;
			for(uint fI=0;fI<faceDefLen;fI++){
				face = &faceDef[fI*FACE_VERTEX_SIZE];
				int *normalPlane = &faceNormals[faceToNormalIdx[fI]*4];

				// skip over faces which are on the exact edge, which cannot by definition be above the line
				uint isVertexInFace = 0;
				for(int i=0;i<FACE_VERTEX_SIZE;i++){
					if(face[i] == vertex1 || face[i] == vertex2){
						isVertexInFace += 1;
					}
				}
				if(isVertexInFace >= 2){
					continue;
				}

				uint isInsideCnt = 0;
				for(int i=0;i<FACE_VERTEX_SIZE;i++){
					tmpF = edgeFunc[fI*FACE_VERTEX_SIZE + i];
					
					if(tmpF >= 0){
						isInsideCnt += 1;
					}
				}
				if(isInsideCnt == FACE_VERTEX_SIZE){
					int faceZ = getPlaneZ(x0, y0, normalPlane, &obj[4*face[0]]);
					if((z0 - faceZ) < -1){
						toDraw = false;
						break;
					}
				}
			}

			// putPixel(x0, y0, color);
			if(toDraw){
				putPixel(x0, y0, color);
			}

			x1 -= dx; 
			if(x1 < 0){
				x1 += dm;
				x0 += sx;
				for(int i=0;i<FACE_VERTEX_SIZE*faceDefLen;i++){
					edgeFunc[i] += edgeDy[i]*sx;
				}
			}

			y1 -= dy;
			if(y1 < 0){
				y1 += dm;
				y0 += sy;
				for(int i=0;i<FACE_VERTEX_SIZE*faceDefLen;i++){
					edgeFunc[i] -= edgeDx[i]*sy;
				}
			}

			z1 -= dz;
			if(z1 < 0){
				z1 += dm;
				z0 += sz;
			}
		}

		
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
	if (SDL_Init(SDL_INIT_VIDEO) == 0) {
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
	float newPosNormal[NORMAL_SIZE*4];
	float txMatrix[4*4];
	float txMatrix2[4*4];

	int newPosInt[VERTEX_SIZE*4];
	int newPosNormalInt[NORMAL_SIZE*4];

	// Simple event loop to keep window open
	SDL_Event e;
	int quit = 0;
	float rotX = -10.0;
	float rotY = 0.0;
	float rotZ = 0.0;

#define ROTATION_INCREMENT	5

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT)
				quit = 1;
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
			}
#endif
		}

#ifndef KEYBOARD_ROTATION
		// rotX -= 0.5;
		// if(rotX > 360) rotX = 0;
		// if(rotX < 0) rotX = 360;
		rotY += 1;
		if(rotY > 360) rotY = 0;
		if(rotY < 0) rotY = 360;
		rotZ += 0.01;
		if(rotZ > 360) rotZ = 0;
		if(rotZ < 0) rotZ = 360;
#endif

		SDL_memset(fb, 0, APP_W * APP_H * sizeof(uint32_t));

		// createTransformMatrix(-20., rot, rot, txMatrix);
		// matrixMult(cubePositions, txMatrix, newPos, (matrixSize_s){CUBE_VERTEX, 4}, (matrixSize_s){4, 4});
		// drawObj(newPos, cubeEdges, CUBE_EDGE);

		createTransformMatrix(rotX, rotY, rotZ, txMatrix);
		matrixMult(toViewCoordsMatrixScale, txMatrix, txMatrix2, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});
		matrixMult(txMatrix2, toViewCoordsMatrixTranslate, txMatrix, (matrixSize_s){4, 4}, (matrixSize_s){4, 4});

		matrixMult(objectNormal, txMatrix2, newPosNormal, (matrixSize_s){NORMAL_SIZE, 4}, (matrixSize_s){4, 4});
		matrixMult(objectVertex, txMatrix, newPos, (matrixSize_s){VERTEX_SIZE, 4}, (matrixSize_s){4, 4});

		for(int i=0;i<VERTEX_SIZE*4;i++){
			newPosInt[i] = (int)newPos[i];
		}
		for(int i=0;i<NORMAL_SIZE*4;i++){
			newPosNormalInt[i] = (int)newPosNormal[i];
		}

		drawObj(newPosInt, objectEdge, EDGE_SIZE, objectFace, FACE_SIZE, newPosNormalInt, objectNormalFaceIdx);

		// Copy fb[] into window surface
		SDL_memcpy(surf->pixels, fb, APP_W * APP_H * sizeof(uint32_t));
		SDL_UpdateWindowSurface(win);

		// rot += 1;
		// if(rot > 360){
		// 	rot = 0;
		// }

		SDL_Delay(25);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
