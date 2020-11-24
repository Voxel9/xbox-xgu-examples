#include "math.h"

#include <stdio.h>
#include <string.h>

#include <math.h>

void mtx_identity(XguMatrix4x4 *mtx) {
    memset(mtx->f, 0, 16*sizeof(float));
    mtx->f[0] = 1.00f;
    mtx->f[5] = 1.00f;
    mtx->f[10] = 1.00f;
    mtx->f[15] = 1.00f;
}

void mtx_multiply(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx1, XguMatrix4x4 mtx2) {
    mtx_out->f[0]  = mtx1.f[0]  * mtx2.f[0] + mtx1.f[1]  * mtx2.f[4] + mtx1.f[2]  * mtx2.f[8]  + mtx1.f[3]  * mtx2.f[12];
    mtx_out->f[1]  = mtx1.f[0]  * mtx2.f[1] + mtx1.f[1]  * mtx2.f[5] + mtx1.f[2]  * mtx2.f[9]  + mtx1.f[3]  * mtx2.f[13];
    mtx_out->f[2]  = mtx1.f[0]  * mtx2.f[2] + mtx1.f[1]  * mtx2.f[6] + mtx1.f[2]  * mtx2.f[10] + mtx1.f[3]  * mtx2.f[14];
    mtx_out->f[3]  = mtx1.f[0]  * mtx2.f[3] + mtx1.f[1]  * mtx2.f[7] + mtx1.f[2]  * mtx2.f[11] + mtx1.f[3]  * mtx2.f[15];
    mtx_out->f[4]  = mtx1.f[4]  * mtx2.f[0] + mtx1.f[5]  * mtx2.f[4] + mtx1.f[6]  * mtx2.f[8]  + mtx1.f[7]  * mtx2.f[12];
    mtx_out->f[5]  = mtx1.f[4]  * mtx2.f[1] + mtx1.f[5]  * mtx2.f[5] + mtx1.f[6]  * mtx2.f[9]  + mtx1.f[7]  * mtx2.f[13];
    mtx_out->f[6]  = mtx1.f[4]  * mtx2.f[2] + mtx1.f[5]  * mtx2.f[6] + mtx1.f[6]  * mtx2.f[10] + mtx1.f[7]  * mtx2.f[14];
    mtx_out->f[7]  = mtx1.f[4]  * mtx2.f[3] + mtx1.f[5]  * mtx2.f[7] + mtx1.f[6]  * mtx2.f[11] + mtx1.f[7]  * mtx2.f[15];
    mtx_out->f[8]  = mtx1.f[8]  * mtx2.f[0] + mtx1.f[9]  * mtx2.f[4] + mtx1.f[10] * mtx2.f[8]  + mtx1.f[11] * mtx2.f[12];
    mtx_out->f[9]  = mtx1.f[8]  * mtx2.f[1] + mtx1.f[9]  * mtx2.f[5] + mtx1.f[10] * mtx2.f[9]  + mtx1.f[11] * mtx2.f[13];
    mtx_out->f[10] = mtx1.f[8]  * mtx2.f[2] + mtx1.f[9]  * mtx2.f[6] + mtx1.f[10] * mtx2.f[10] + mtx1.f[11] * mtx2.f[14];
    mtx_out->f[11] = mtx1.f[8]  * mtx2.f[3] + mtx1.f[9]  * mtx2.f[7] + mtx1.f[10] * mtx2.f[11] + mtx1.f[11] * mtx2.f[15];
    mtx_out->f[12] = mtx1.f[12] * mtx2.f[0] + mtx1.f[13] * mtx2.f[4] + mtx1.f[14] * mtx2.f[8]  + mtx1.f[15] * mtx2.f[12];
    mtx_out->f[13] = mtx1.f[12] * mtx2.f[1] + mtx1.f[13] * mtx2.f[5] + mtx1.f[14] * mtx2.f[9]  + mtx1.f[15] * mtx2.f[13];
    mtx_out->f[14] = mtx1.f[12] * mtx2.f[2] + mtx1.f[13] * mtx2.f[6] + mtx1.f[14] * mtx2.f[10] + mtx1.f[15] * mtx2.f[14];
    mtx_out->f[15] = mtx1.f[12] * mtx2.f[3] + mtx1.f[13] * mtx2.f[7] + mtx1.f[14] * mtx2.f[11] + mtx1.f[15] * mtx2.f[15];
}

void mtx_transpose(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in) {
    XguMatrix4x4 temp;
    
    temp.f[0]  = mtx_in.f[0];
    temp.f[1]  = mtx_in.f[4];
    temp.f[2]  = mtx_in.f[8];
    temp.f[3]  = mtx_in.f[12];
    temp.f[4]  = mtx_in.f[1];
    temp.f[5]  = mtx_in.f[5];
    temp.f[6]  = mtx_in.f[9];
    temp.f[7]  = mtx_in.f[13];
    temp.f[8]  = mtx_in.f[2];
    temp.f[9]  = mtx_in.f[6];
    temp.f[10] = mtx_in.f[10];
    temp.f[11] = mtx_in.f[14];
    temp.f[12] = mtx_in.f[3];
    temp.f[13] = mtx_in.f[7];
    temp.f[14] = mtx_in.f[11];
    temp.f[15] = mtx_in.f[15];
    
    memcpy(mtx_out, &temp, sizeof(XguMatrix4x4));
}

void mtx_inverse(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in) {
    XguMatrix4x4 temp;
    
    mtx_transpose(&temp, mtx_in);
    temp.f[3] = 0.00f;
    temp.f[7] = 0.00f;
    temp.f[11] = 0.00f;
    temp.f[12] = -(mtx_in.f[12] * temp.f[0] + mtx_in.f[13] * temp.f[4] + mtx_in.f[14] * temp.f[8]);
    temp.f[13] = -(mtx_in.f[12] * temp.f[1] + mtx_in.f[13] * temp.f[5] + mtx_in.f[14] * temp.f[9]);
    temp.f[14] = -(mtx_in.f[12] * temp.f[2] + mtx_in.f[13] * temp.f[6] + mtx_in.f[14] * temp.f[10]);
    temp.f[15] = 1.00f;
    
    memcpy(mtx_out, &temp, sizeof(XguMatrix4x4));
}

void mtx_translate(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 translate) {
    XguMatrix4x4 temp;
    
    mtx_identity(&temp);
    temp.f[12] = translate.f[0];
    temp.f[13] = translate.f[1];
    temp.f[14] = translate.f[2];
    mtx_multiply(mtx_out, mtx_in, temp);
}

void mtx_rotate(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 rotate) {
    XguMatrix4x4 temp;
    
    // Z
    mtx_identity(&temp);
    temp.f[0] =  cosf(rotate.f[2]);
    temp.f[1] =  sinf(rotate.f[2]);
    temp.f[4] = -sinf(rotate.f[2]);
    temp.f[5] =  cosf(rotate.f[2]);
    mtx_multiply(mtx_out, mtx_in, temp);
    
    // Y
    mtx_identity(&temp);
    temp.f[0]  =  cosf(rotate.f[1]);
    temp.f[2]  = -sinf(rotate.f[1]);
    temp.f[8]  =  sinf(rotate.f[1]);
    temp.f[10] =  cosf(rotate.f[1]);
    mtx_multiply(mtx_out, *mtx_out, temp);
    
    // X
    mtx_identity(&temp);
    temp.f[5]  =  cosf(rotate.f[0]);
    temp.f[6]  =  sinf(rotate.f[0]);
    temp.f[9]  = -sinf(rotate.f[0]);
    temp.f[10] =  cosf(rotate.f[0]);
    mtx_multiply(mtx_out, *mtx_out, temp);
}

void mtx_scale(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 scale)  {
    XguMatrix4x4 temp;
    
    mtx_identity(&temp);
    temp.f[0] = scale.f[0];
    temp.f[5] = scale.f[1];
    temp.f[10] = scale.f[2];
    mtx_multiply(mtx_out, mtx_in, temp);
}

void mtx_world_view(XguMatrix4x4 *world_view, XguVec4 translate, XguVec4 rotate) {
    // Invert translate/rotate vectors
    translate.f[0] = -translate.f[0];
    translate.f[1] = -translate.f[1];
    translate.f[2] = -translate.f[2];
    
    rotate.f[0] = -rotate.f[0];
    rotate.f[1] = -rotate.f[1];
    rotate.f[2] = -rotate.f[2];
    
    mtx_identity(world_view);
    mtx_translate(world_view, *world_view, translate);
    mtx_rotate(world_view, *world_view, rotate);
}

void mtx_view_screen(XguMatrix4x4 *view_screen, float aspect, float fov, float near, float far) {
    fov *= 0.01745329251f;
    
    mtx_identity(view_screen);
    view_screen->f[0]  = fov/aspect;
    view_screen->f[5]  = fov;
    view_screen->f[10] = far / (near - far);
    view_screen->f[11] = -1.0f;
    view_screen->f[14] = -(far * near) / (far - near);
    view_screen->f[15] = 0.0f;
}

void mtx_viewport(XguMatrix4x4 *mtx_out, float x, float y, float width, float height, float z_min, float z_max) {
    mtx_identity(mtx_out);
    
    mtx_out->f[0]  = width/2.0f;
    mtx_out->f[5]  = height/-2.0f;
    mtx_out->f[10] = (z_max - z_min)/2.0f;
    mtx_out->f[12] = x + width/2.0f;
    mtx_out->f[13] = y + height/2.0f;
    mtx_out->f[14] = (z_min + z_max)/2.0f;
}
