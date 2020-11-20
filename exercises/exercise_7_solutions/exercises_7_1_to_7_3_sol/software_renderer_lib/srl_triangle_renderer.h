//
// Created by Henrique Galvan Debarba on 2019-09-29.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_OGLTRIANGLERENDERER_H
#define GRAPHICSPROGRAMMINGEXERCISES_OGLTRIANGLERENDERER_H

#include <algorithm>
#include "rasterizer/trianglerasterizer.h"

namespace srl {

    class TriangleRenderer : public Renderer {
    public:
        bool m_clipToFrustum = true;
        bool m_cullBackFaces = true;

    private:

        void processPrimitives (const std::vector<vertex> &inVts, unsigned int width, unsigned int height, std::vector<fragment> &outFrs) override{
            // 2.1. create the primitives
            assemblePrimitives(inVts);

            // 2.2. keep primitives in the visible volume
            if(m_clipToFrustum) clipPrimitives();

            // NEW!
            // scale down the primitives so that the clipping is visible in the screen space
            // TODO can be removed once clipping is working
            glm::mat4 scale = glm::scale(0.75f, 0.75f, 0.75f);
            for (auto & tri : m_primitives) {
                tri.v1.pos = scale * tri.v1.pos;
                tri.v2.pos = scale * tri.v2.pos;
                tri.v3.pos = scale * tri.v3.pos;
            }

            // 2.3. move vertices to normalized device coordinates
            divideByW();

            // 2.4. normalized device coordinates to screen space
            toScreenSpace(width, height);

            // 2.5. reject primitives that are not facing towards the camera
            if(m_cullBackFaces) backfaceCulling();

            // 2.6. rasterization (generate fragments)
            rasterPrimitives(outFrs);
        }

        // 2.1. create triangle primitives
        void assemblePrimitives(const std::vector<vertex> &vts) {
            m_primitives.clear();
            m_primitives.reserve(vts.size()/3);

            for(int i = 0, size = vts.size()-2; i < size; i+=3){
                triangle t;
                t.v1 = vts[i];
                t.v2 = vts[i+1];
                t.v3 = vts[i+2];

                m_primitives.push_back(t);
            }
        }

        bool clipTriangle(triangle &tIn, int i){//}, triangle &tOut1, triangle &tOut2){
            // index to x, y or z coordinate (x=0, y=1, z=2)
            int idx = i % 3;
            // we check if the variable is in the range of the clipping plane using w
            // we need to test if w >= x,y,z >= -w, and clip when x,y,z > w or x,y,z < -w
            // we can rewrite the latter with x,y,z * -1 > w
            // so we need to multiply x,y,z by -1 when testing against the planes at -w
            // planes 0, 1 and 2 are positive w, planes 3, 4 and 5 are negative w
            int wMult = i > 2 ? -1 : 1;

            // positions of the three vertex
            glm::vec4 p1 = tIn.v1.pos;
            glm::vec4 p2 = tIn.v2.pos;
            glm::vec4 p3 = tIn.v3.pos;

            // store a pointer to the vertices in and out the desired half-space
            vertex* inVts[3]; int inCount = 0;
            vertex* outVts[3]; int outCount = 0;
            int outIdx;

            // test if the points are in the valid
            if(p1[idx] * wMult > p1.w) {outVts[outCount] = &tIn.v1; outCount++; outIdx = 0;}
            else {inVts[inCount] = &tIn.v1; inCount++;}
            if(p2[idx] * wMult > p2.w) {outVts[outCount] = &tIn.v2; outCount++; outIdx = 1;}
            else {inVts[inCount] = &tIn.v2; inCount++;}
            if(p3[idx] * wMult > p3.w) {outVts[outCount] = &tIn.v3; outCount++; outIdx = 2;}
            else {inVts[inCount] = &tIn.v3; inCount++;}


            if (outCount == 0) {
                // whole triangle in the valid side of the half-space (or over the plane)
                return true;
            }
            else if (outCount == 3) {
                // whole triangle in the invalid side of the half-space
                // reject this triangle
                tIn.rejected = true;
                return false;
            }
            else if (outCount == 2) {   // two vertices in the invalid side of the half-space
                // vector from in position to first out position
                glm::vec4 inOutVec = outVts[0]->pos - inVts[0]->pos;
                // find the weight t
                float t = (inVts[0]->pos[idx] - inVts[0]->pos.w * wMult) / (inOutVec.w * wMult - inOutVec[idx]);
                // compute edge intersection 1
                vertex edgeVtx1 = (*inVts[0]) + (*outVts[0] - *inVts[0]) * t;

                // vector from in position to second out position
                inOutVec = outVts[1]->pos - inVts[0]->pos;
                // find the weight t
                t = (inVts[0]->pos[idx] - inVts[0]->pos.w * wMult) / (inOutVec.w * wMult - inOutVec[idx]);
                // compute edge intersection 2
                vertex edgeVtx2 = (*inVts[0]) + (*outVts[1] - *inVts[0]) * t;

                // update the two triangle vertices in the invalid half-space
                *(outVts[0]) = edgeVtx1;
                *(outVts[1]) = edgeVtx2;
            }
            else if (outCount == 1) {   // one vertices in the invalid side of the half-space

                // vector from first in position to out position
                glm::vec4 inOutVec = outVts[0]->pos - inVts[0]->pos;
                // find the weight t
                float t = (inVts[0]->pos[idx] - inVts[0]->pos.w * wMult) / (inOutVec.w * wMult - inOutVec[idx]);
                // compute edge intersection 1
                vertex edgeVtx1 = (*inVts[0]) + (*outVts[0] - *inVts[0]) * t;

                // vector from second in position to out position
                inOutVec = outVts[0]->pos - inVts[1]->pos;
                // find the weight t
                t = (inVts[1]->pos[idx] - inVts[1]->pos.w * wMult) / (inOutVec.w * wMult - inOutVec[idx]);
                // compute edge intersection 2
                vertex edgeVtx2 = (*inVts[1]) + (*outVts[0] - *inVts[1]) * t;

                // update the location of the vertex in the invalid side of the half-space
                *outVts[0] = edgeVtx1;

                // we have fixed the triangle that was already stored, now lets create the triangle that is missing
                // using the two edge points and the second in vertex
                triangle newT;
                // TODO 7.3 make sure the winding order of new triangles is correct
                if(outIdx == 0){newT.v1 = *inVts[1]; newT.v2 = edgeVtx2; newT.v3 = edgeVtx1;}
                else if(outIdx == 1){newT.v1 =  *inVts[1]; newT.v2 = edgeVtx1; newT.v3 = edgeVtx2;}
                else {newT.v1 = edgeVtx1; newT.v2 = *inVts[1]; newT.v3 = edgeVtx2;}

                m_primitives.push_back(newT);
            }

            return true;
        }


        // 2.2. clip primitives so that they are contained within the render volume
        void clipPrimitives() {
            // TODO Exercise 7.1 - implement triangle clipping
            for (int side = 0; side < 6; side ++){
                for(int i = 0, size = m_primitives.size(); i < size; i++){
                    if (!m_primitives[i].rejected)
                        clipTriangle(m_primitives[i], side);
                }
            }
        }

        // 2.3. perspective division (canonical perspective volume to normalized device coordinates)
        void divideByW() {
            for(auto &tri : m_primitives) {
                // we divide all parameters due to hyperbolic interpolation
                tri.v1 = tri.v1 / tri.v1.pos.w;
                tri.v2 = tri.v2 / tri.v2.pos.w;
                tri.v3 = tri.v3 / tri.v3.pos.w;
            }
        }

        // 2.4. normalized device coordinates to screen space
        void toScreenSpace(int width, int height)  {
            float halfW = width / 2;
            float halfH = width / 2;
            glm::mat4 toWindowSpace = glm::scale(halfW, halfH, 1.f) * glm::translate(1.f, 1.f, 0.f);
            for(auto &tri : m_primitives) {
                tri.v1.pos = toWindowSpace * tri.v1.pos;
                tri.v2.pos = toWindowSpace * tri.v2.pos;
                tri.v3.pos = toWindowSpace * tri.v3.pos;
            }
        }


        // 2.5. only draw triangles in a counterclockwise winding order and facing the camera
        void backfaceCulling(){
            // TODO Exercise 7.2 - implement backface culling
            for(auto &tri : m_primitives) {
                // two vectors along the edges of the triangle
                glm::vec3 v1 = tri.v2.pos - tri.v1.pos;
                glm::vec3 v2 = tri.v3.pos - tri.v1.pos;

                // z component of the normal in the NDC
                float nz = v1.x * v2.y - v1.y * v2.x;

                // bigger than 0 means the normal is not pointing towards the camera
                if (nz > 0) {
                    tri.rejected = true;
                }
            }
        }

        // 2.6. rasterization (generate fragments)
        void rasterPrimitives(std::vector<fragment> &frs) {
            frs.clear();

            for(auto &tri : m_primitives) {
                // skip this primitive
                if(tri.rejected)
                    continue;

                // create primitive rasterizer
                triangle_rasterizer rasterizer(tri.v1, tri.v2, tri.v3);

                // generate the fragments
                while (rasterizer.more_fragments()) {
                    srl::fragment frag;
                    frag.posX = rasterizer.x();
                    frag.posY = rasterizer.y();

                    vertex vtx = rasterizer.getCurrent();
                    frag.depth = vtx.pos.z;
                    vtx = vtx/vtx.one; // hyperbolic interpolation
                    frag.col = vtx.col;

                    frs.push_back(frag);
                    rasterizer.next_fragment();
                }
            }
        }

        // list of triangle primitives.
        std::vector<triangle> m_primitives;
    };
};


#endif //GRAPHICSPROGRAMMINGEXERCISES_OGLTRIANGLERENDERER_H
