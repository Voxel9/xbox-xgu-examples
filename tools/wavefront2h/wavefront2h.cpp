#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x;
    float y;
    float z;
} vec3;

int main(int argc, char** argv) {
    FILE *fp = fopen(argv[1], "r");
    if(!fp) {
        printf("Failed to open wavefront OBJ file!\n");
        return 1;
    }
    
    if(!argv[2]) {
        printf("You need to specify output h file after input file!\n");
        return 1;
    }
    
    int uv_mul_x = 1, uv_mul_y = 1;
    
    if(argv[3]) {
        sscanf(argv[3], "%ix%i", &uv_mul_x, &uv_mul_y);
    }
    else {
        printf("Warning: Texture width/height not specified - defaulting to UV coords (specify texture size by passing WxH as 3rd argument)\n");
    }
    
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<vec3> temp_vertices;
    std::vector<vec2> temp_uvs;
    std::vector<vec3> temp_normals;
    
    std::vector<vec3> out_vertices;
    std::vector<vec2> out_uvs;
    std::vector<vec3> out_normals;
    
    while(1) {
        char lineHeader[128];
        
        int res = fscanf(fp, "%s", lineHeader);
        
        if(res == EOF)
            break;
        
        if(!strcmp(lineHeader, "v")) {
            vec3 vertex;
            fscanf(fp, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            temp_vertices.push_back(vertex);
        }
        else if(!strcmp(lineHeader, "vt")) {
            vec2 uv;
            fscanf(fp, "%f %f\n", &uv.x, &uv.y);
            temp_uvs.push_back(uv);
        }
        else if(!strcmp(lineHeader, "vn")) {
            vec3 normal;
            fscanf(fp, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);
        }
        else if(!strcmp(lineHeader, "f")) {
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            
            int matches = fscanf(fp, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            
            if (matches != 9) {
                printf("Failed to parse faces. Try using 3DS Max/Blender\n");
                return false;
            }
            
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
    }
    
    fclose(fp);
    
    for(unsigned int i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        vec3 vertex = temp_vertices[vertexIndex-1];
        out_vertices.push_back(vertex);
    }
    
    for(unsigned int i = 0; i < uvIndices.size(); i++) {
        unsigned int uvIndex = uvIndices[i];
        vec2 uv = temp_uvs[uvIndex-1];
        out_uvs.push_back(uv);
    }
    
    for(unsigned int i = 0; i < normalIndices.size(); i++) {
        unsigned int normalIndex = normalIndices[i];
        vec3 normal = temp_normals[normalIndex-1];
        out_normals.push_back(normal);
    }
    
    FILE *outfile = fopen(argv[2], "w");
    
    fprintf(outfile, "static const Vertex vertices[] = {\n");
    
    for(unsigned int i = 0; i < out_vertices.size(); i++) {
        fprintf(outfile, "    { {%f, %f, %f}, {%f, %f}, {%f, %f, %f} },\n",
            out_vertices[i].x, out_vertices[i].y, out_vertices[i].z,
            out_uvs[i].x * uv_mul_x, out_uvs[i].y * uv_mul_y,
            out_normals[i].x, out_normals[i].y, out_normals[i].z
        );
    }
    
    fprintf(outfile, "};\n");
    
    fclose(outfile);
    return 0;
}