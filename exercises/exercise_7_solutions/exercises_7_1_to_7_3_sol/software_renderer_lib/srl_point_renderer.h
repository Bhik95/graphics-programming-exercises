//
// Created by Henrique Galvan Debarba on 2019-09-29.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_OGLPOINTRENDERER_H
#define GRAPHICSPROGRAMMINGEXERCISES_OGLPOINTRENDERER_H

#include "srl_renderer.h"

namespace srl {

    class PointRenderer : public Renderer {
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
            for (auto & point : m_primitives) {
                point.v.pos = scale * point.v.pos;
            }

            // 2.3. move vertices to normalized device coordinates
            divideByW();

            // 2.4. normalized device coordinates to screen space
            toScreenSpace(width, height);

            // 2.5. NO back-face culling for points

            // 2.6. rasterization (generate fragments)
            rasterPrimitives(outFrs);
        }

        // 2.1. create point primitives
        void assemblePrimitives(const std::vector<vertex> &vts) {
            m_primitives.clear();
            m_primitives.reserve(vts.size());

            for(int i = 0, size = vts.size(); i < size; i++){
                point p;
                p.v = vts[i];

                m_primitives.push_back(p);
            }
        }

        // 2.2. reject points that are out of the render volume
        void clipPrimitives() {
            for(auto it = m_primitives.begin(); it < m_primitives.end(); it++){
                glm::vec4 pos = it->v.pos;
                // we only want to render points with x,y and z in the range [-w,w]
                if( abs(pos.x) > abs(pos.w) || abs(pos.y) > abs(pos.w) || abs(pos.z) > abs(pos.w) ) {
                    it->rejected = true;
                }
            }
        }

        // 2.3. perspective division (canonical perspective volume to normalized device coordinates)
        void divideByW() {
            for(auto &point : m_primitives) {
                point.v.pos.z /= point.v.pos.w;
                point.v = point.v / point.v.pos.w;
            }
        }

        // 2.4. normalized device coordinates to screen space
        void toScreenSpace(int width, int height)  {
            float halfW = width / 2;
            float halfH = width / 2;
            glm::mat4 toWindowSpace = glm::scale(halfW, halfH, 1.f) * glm::translate(1.f, 1.f, 0.f);
            for(auto &point : m_primitives) {
                point.v.pos = toWindowSpace * point.v.pos;
            }
        }

        // 2.6. rasterization (generate fragments)
        void rasterPrimitives(std::vector<fragment> &outFrs) {
            outFrs.clear();
            outFrs.reserve(m_primitives.size());

            // convert points into fragments
            for(auto &point : m_primitives) {
                if (point.rejected)
                    continue;

                fragment fr;
                vertex v = point.v;
                fr.posX = (int) (v.pos.x + .5f);
                fr.posY = (int) (v.pos.y + .5f);

                v = v/v.one; // hyperbolic interpolation
                fr.depth = v.pos.z;
                fr.col = v.col;

                outFrs.push_back(fr);
            }
        }

        // list of point primitives.
        std::vector<point> m_primitives;
    };
};

#endif //GRAPHICSPROGRAMMINGEXERCISES_OGLPOINTRENDERER_H
