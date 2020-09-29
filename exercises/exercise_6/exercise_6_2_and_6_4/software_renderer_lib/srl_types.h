//
// Created by Henrique Galvan Debarba on 2019-09-29.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_OGLTYPES_H
#define GRAPHICSPROGRAMMINGEXERCISES_OGLTYPES_H

#include <glm/glm.hpp>

namespace srl {
    struct point;
    struct line;
    struct triangle;

    struct color{
        // NEW!
        // float type instead of 8bit unsigned int, this makes it easier to interpolate colors
        float r, g, b, a;

        // predefined colors
        static color white() { return {1, 1, 1, 1}; }
        static color red() { return {1, 0, 0, 1}; }
        static color green() { return {0, 1, 0, 1}; }
        static color blue() { return {0, 0, 1, 1}; }
        static color grey() { return {.5f, .5f, .5f, 1}; }
        static color black() { return {0, 0, 0, 1}; }

        // color as four 8 bits uint, packed in a 32 bits uint
        inline std::uint32_t getRGBA32() const{
            return (uint32_t(255*r)) | (uint32_t(255*g) << 8) |
                   (uint32_t(255*b) << 16) | (uint32_t(255*a) << 24);
        }
    };

    // vertex definition, you can think of that as the in and out variables of the vertex shader
    struct vertex {

        glm::vec4 pos;
        color col;
        float one;


        vertex() : one(1.0f) {}

        friend vertex operator/ (vertex v, float sc){
            v.pos /= sc;
            v.col.r /= sc; v.col.g /= sc; v.col.b /= sc; v.col.a /= sc;
            v.one /= sc;
            return v;
        }

        friend vertex operator* (vertex v, float sc){
            v.pos *= sc;
            v.col.r *= sc; v.col.g *= sc; v.col.b *= sc; v.col.a *= sc;
            v.one *= sc;
            return v;
        }

        friend vertex operator- (vertex v1, const vertex &v2){
            v1.pos -= v2.pos;
            v1.col.r -= v2.col.r; v1.col.g -= v2.col.g; v1.col.b -= v2.col.b; v1.col.a -= v2.col.a;
            v1.one -= v2.one;
            return v1;
        }

        friend vertex operator+ (vertex v1, const vertex &v2){
            v1.pos += v2.pos;
            v1.col.r += v2.col.r; v1.col.g += v2.col.g; v1.col.b += v2.col.b; v1.col.a += v2.col.a;
            v1.one += v2.one;
            return v1;
        }

    };

    // primitives
    struct point{
        vertex v;
        bool rejected = false;
    };

    struct triangle{
        vertex v1;
        vertex v2;
        vertex v3;
        bool rejected = false;
    };

    struct line {
        vertex v1;
        vertex v2;
        bool rejected = false;
    };

    // fragment definition, you can think of that as the in and out variables of the fragment shader
    struct fragment {
        color col;
        int posX;
        int posY;
        float depth;


    };


}
#endif //GRAPHICSPROGRAMMINGEXERCISES_OGLTYPES_H
