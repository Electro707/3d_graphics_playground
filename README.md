# 3D Graphics Playground

This is a personal playground where I play around with 3d computer graphics.

The idea is for this code to eventually make itself unto an embedded system with limited memory

![DemoVideo1](.readmeImg/Screencast_20250906_224342.webm)


# Files:
- cube.c: Rotates a cube
- camera.c: Rotates a simple camera model
- camera_face.c: Same as `camera.c`, but includes hidden line removal
- makefile: Used to build. Current targets are named the same as the C files

- objCamera.h: Header file that includes data for the camera model
- objCube.h: Header file that includes data for a basic cube

- camera1.blend: Blender file for the camera model
- camera1_triangle.blend: Same as `camera1.blend`, but pre-triangulated

- convertObjToArray.py: Python file that converts an OBJ file to what should be in a header file for the C files above to use. First and only argument is the obj file to read

- `fixedPoint/`: Where I was playing around with fixed point math, before it turns out it doesn't make that much of a difference

# License
This project is licensed under [GPLv3](LICENSE.md)