!!VP1.1

# v0 - position
# v8 - texcoord0

# c0-c3 - Composite matrix

# Position
DP4   o[HPOS].x, v[0], c[0];
DP4   o[HPOS].y, v[0], c[1];
DP4   o[HPOS].z, v[0], c[2];
DP4   o[HPOS].w, v[0], c[3];

# Texcoord0
MOV   o[TEX0].xy, v[8];

# Viewport/Clipping
RCC   R1.x, R12.w;
MUL   o[HPOS].xyz, R12, R1.x;

END
