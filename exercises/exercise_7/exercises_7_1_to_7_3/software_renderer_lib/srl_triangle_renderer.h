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


        // 2.2. clip primitives so that they are contained within the render volume
        void clipPrimitives() {
            // TODO Exercise 7.1 - implement triangle clipping

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
