#include "draw.h"

// Helper function to allow the drawing of more than 256 vertices at once.
void draw_vertex_arrays(XguPrimitiveType mode, unsigned int count) {
    unsigned int batches = count / 255;
    unsigned int remainder = count % 255;
    
    unsigned int pos = 0;
    
    // Process verts in small batches of 255
    for(int i = 0; i < batches; i++) {
        xgux_draw_arrays(mode, pos, 255);
        pos += 255;
    }
    
    // Process remaining verts
    xgux_draw_arrays(mode, pos, remainder);
}
