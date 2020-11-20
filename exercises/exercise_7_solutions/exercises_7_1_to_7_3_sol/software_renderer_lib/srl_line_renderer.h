//
// Created by Henrique Galvan Debarba on 2019-09-27.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_OGLLINERENDERER_H
#define GRAPHICSPROGRAMMINGEXERCISES_OGLLINERENDERER_H

#include "srl_renderer.h"
#include "rasterizer/linerasterizer.h"

namespace srl {

    class LineRenderer : public Renderer {
    public:
        bool m_clipToFrustum = true;

    private:
        void processPrimitives (const std::vector<vertex> &inVts, unsigned int width, unsigned int height, std::vector<fragment> &outFrs) override{
            // 2.1. create the primitives
            assemblePrimitives(inVts);

            // 2.2. keep primitives in the visible volume
            if(m_clipToFrustum)
                clipPrimitives();

            // NEW!
            // scale down the primitives so that the clipping is visible in the screen space
            // TODO can be removed once clipping is working
            glm::mat4 scale = glm::scale(0.75f, 0.75f, 0.75f);
            for (auto & line : m_primitives) {
                line.v1.pos = scale * line.v1.pos;
                line.v2.pos = scale * line.v2.pos;
            }

            // 2.3. move vertices to normalized device coordinates
            divideByW();

            // 2.4. normalized device coordinates to screen space
            toScreenSpace(width, height);

            // 2.5. NO back-face culling for points

            // 2.6. rasterization (generate fragments)
            rasterPrimitives(outFrs);
        }


        // 2.1. create line primitives
        void assemblePrimitives(const std::vector<vertex> &vts) {
            m_primitives.clear();
            // make sure a single allocation will happen
            m_primitives.reserve(vts.size()/3 * (wireframe ? 3 : 1));
            int increment =  wireframe ? 3 : 2;
            for(int i = 0, size = vts.size()-1; i < size; i += increment){
                line l;
                l.v1 = vts[i];
                l.v2 = vts[i+1];
                m_primitives.push_back(l);
                if(wireframe) {
                    l.v1 = vts[i + 1];
                    l.v2 = vts[i + 2];
                    m_primitives.push_back(l);
                    l.v1 = vts[i + 2];
                    l.v2 = vts[i];
                    m_primitives.push_back(l);
                }
            }
        }

        void clipLine(line &l, int side){
            vertex &v1 = l.v1;
            vertex &v2 = l.v2;

            // index to x, y or z coordinate (x=0, y=1, z=2)
            int idx = side % 3;
            // we check if the variable is in the range of the clipping plane using w
            // we need to test if w >= x,y,z >= -w, and clip when x,y,z > w or x,y,z < -w
            // we can rewrite the latter with x,y,z * -1 > w
            // so we need to multiply x,y,z by -1 when testing against the planes at -w
            // planes 0, 1 and 2 are positive w, planes 3, 4 and 5 are negative w
            float wMult = side > 2 ? -1.0f : 1.0f;

            glm::vec4 p1 = v1.pos;
            glm::vec4 p2 = v2.pos;
            int outCount = (p1[idx] * wMult > p1.w) + (p2[idx] * wMult > p2.w);
            if( outCount == 2){
                // the line is outside the frustum, we don't need to draw it
                l.rejected = true;
                return;
            }
            else if (outCount == 0){
                // no need to clip against this plane
                return;
            }
            else { // (outCount == 1)  one vertex is inside and the other is outside

                // vector from position p1 to p2
                glm::vec4 p1p2vec = p2 - p1;

                // proportion t that added to p1 will give the point where coordinates p1[idx] + p1p2vec[idx]*t == w , for idx = x, y or z
                float denom = p1p2vec.w * wMult - p1p2vec[idx];
                float t = (p1[idx] - p1.w * wMult) / denom;

                // interpolate and update the value of one of the variables
                vertex &vTarget = p1[idx] * wMult > p1.w ? v1 : v2;
                vTarget = v1 + (v2 - v1) * t;
            }
        }

        // 2.2. clip primitives so that they are contained within the render frustum
        void clipPrimitives()  {
            // repeat for the six planes of the viewing frustum
            for (int side = 0; side < 6; side ++){
                for(int i = 0, size = m_primitives.size(); i < size; i++){
                    if (!m_primitives[i].rejected)
                        clipLine(m_primitives[i], side);
                }
            }
        }

        // 2.3. perspective division (canonical perspective volume to normalized device coordinates)
        void divideByW() {
            for(auto &line : m_primitives) {
                line.v1.pos.z /= line.v1.pos.w;
                line.v1 = line.v1 / line.v1.pos.w;
                line.v2.pos.z /= line.v2.pos.w;
                line.v2 = line.v2 / line.v2.pos.w;
            }
        }

        // 2.4. normalized device coordinates to screen space
        void toScreenSpace(int width, int height)  {
            float halfW = width / 2;
            float halfH = width / 2;
            glm::mat4 toWindowSpace = glm::scale(halfW, halfH, 1.f) * glm::translate(1.f, 1.f, 0.f);
            for(auto &line : m_primitives) {
                line.v1.pos = toWindowSpace * line.v1.pos;
                line.v2.pos = toWindowSpace * line.v2.pos;
            }
        }

        // 2.6. rasterization (generate fragments)
        void rasterPrimitives(std::vector<fragment> &outFrs) {
            outFrs.clear();

            for(auto &line : m_primitives) {
                // skip current primitive?
                if(line.rejected)
                    continue;

                LineRasterizer rasterizer(line.v1, line.v2);

                while (rasterizer.MoreFragments()) {
                    srl::fragment frag;

                    frag.posX = rasterizer.x();
                    frag.posY = rasterizer.y();

                    vertex vtx = rasterizer.GetCurrent();
                    vtx = vtx/vtx.one; // hyperbolic interpolation
                    frag.depth = vtx.pos.z;
                    frag.col = vtx.col;

                    outFrs.push_back(frag);
                    rasterizer.NextFragment();
                }
            }
        }


        // lists of line primitives.
        std::vector<line> m_primitives;
        bool wireframe = false;
    };

}

#endif //GRAPHICSPROGRAMMINGEXERCISES_OGLLINERENDERER_H
