//
// Created by Henrique Galvan Debarba on 2019-09-27.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_RENDERER_H
#define GRAPHICSPROGRAMMINGEXERCISES_RENDERER_H

#include <vector>
#include <algorithm>
#include "glm/glm.hpp"
#include "srl_frame_buffer.h"
#include "srl_types.h"

namespace srl {

    class Renderer {

    public:

        // render vertices with mvp transformation in the fb framebuffer
        virtual void render(const std::vector<vertex> &vts, const glm::mat4 &mvp, FrameBuffer <uint32_t> &fb, FrameBuffer <float> &db) {
            // will copy all vertices since vts is const
            m_vts = vts;
            // 1. our vertex shader
            processVertices(mvp, m_vts);

            // 2. the fixed part of the pipeline
            processPrimitives(m_vts, fb.width(), fb.height(), m_frs);

            // 3. our fragment shader
            processFragments(m_frs);

            // 4. fragment operations and copy color to the frame buffer
            writeToFrameBuffer(m_frs, fb, db);
        }

    private:


        virtual void processPrimitives (const std::vector<vertex> &inVts, unsigned int width, unsigned int height, std::vector<fragment> &outFrs) = 0;



        // perform vertex operations in the vertex stream (i.e. the equivalent to a vertex shader)
        void processVertices(const glm::mat4 &mvp, std::vector<vertex> &vInOut) {
            for (auto &vtx : vInOut){
                // this is the equivalent to a vertex shader
                vertex v = vtx;
                // transform position
                v.pos = mvp * v.pos;
                // copy color
                v.col = v.col;
                // save it in the list
                vtx = v;
            }
        }

        // perform fragment operations in the fragment stream (i.e. fragment shader)
        void processFragments(std::vector<fragment>& fInOut) {
            // fragment shader - not necessary for now since we are not modifying the color

        }

        // fragment operations and copy color to frame buffer
        void writeToFrameBuffer(const std::vector<fragment> &frs, FrameBuffer <uint32_t> &fb, FrameBuffer <float> &db) {
			int width = fb.width();
			int height = fb.height();
            for (int i = 0, size = frs.size(); i < size; i++) {
				int posX = frs[i].posX;
				int posY = frs[i].posY;

                // make sure it is within framebuffer range (it won't be if we do not clip)
				if (posX < 0 || posX >= width || posY < 0 || posY >= height)
					continue;

				// get 1D index in the framebuffer
				int index = db.indexAt(posX, posY);
				
				// blending test and z/depth-buffer will come here
				// set the color of the pixel in the frame buffer
				if (frs[i].depth < db[index]) {
					fb[index] = frs[i].col.getRGBA32();
					db[index] = frs[i].depth;
				}
            }
        }

        // lists of vertices and fragments. These are here to keep the allocated memory.
        std::vector<vertex> m_vts;
        std::vector<fragment> m_frs;
    };

}

#endif //GRAPHICSPROGRAMMINGEXERCISES_RENDERER_H
