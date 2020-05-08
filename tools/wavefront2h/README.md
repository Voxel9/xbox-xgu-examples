# Wavefront2h

This is a quick and dirty C++ program to process wavefront OBJ files into C header files. Primarily here for aiding in sample development.  

Wavefront2h produces a _triangle list_ for quick and easy usage, but consumes more memory than using indices, so you may wish to consider implementing your own optimised model loader instead of relying on this tool.  

For the purpose of nxdk/xgu, this tool allows you to enter the width and height of the OBJ model's texture, which will output texcoords in pixel space instead of UV space.

## Build
`g++ wavefront2h.cpp -o wavefront2h.exe`

## Usage
`wavefront2h <OBJ file> <output H file> WxH`  
(Replace 'W' with texture width and 'H' with texture height, both in pixels)
