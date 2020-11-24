#ifndef __MATH_H__
#define __MATH_H__

#include <xgu/xgu.h>

void mtx_identity(XguMatrix4x4 *mtx);

void mtx_multiply(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx1, XguMatrix4x4 mtx2);
void mtx_transpose(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in);
void mtx_inverse(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in);

void mtx_translate(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 translate);
void mtx_rotate(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 rotate);
void mtx_scale(XguMatrix4x4 *mtx_out, XguMatrix4x4 mtx_in, XguVec4 scale);

void mtx_world_view(XguMatrix4x4 *world_view, XguVec4 translate, XguVec4 rotate);
void mtx_view_screen(XguMatrix4x4 *view_screen, float aspect, float fov, float near, float far);
void mtx_viewport(XguMatrix4x4 *mtx_out, float x, float y, float width, float height, float z_min, float z_max);

#endif
