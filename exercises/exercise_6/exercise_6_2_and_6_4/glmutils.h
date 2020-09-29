#ifndef __RASTERMAN_GLM_UTILS_H__
#define __RASTERMAN_GLM_UTILS_H__

/**
 * \file glmutils.h
 */

#include <iostream>
#include <stdexcept>
#include <cmath>
#include <string>
#include <cctype>
#include <iomanip>

// Use gtx_transform
#define GLM_ENABLE_EXPERIMENTAL
// Use radians by default
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/integer.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>

//#include "traceinfo.h"

/**
 * \namespace glm
 *
 * This library expands the OpenGL glm mathematical library with a number of utility functions. \n
 * There are also operators to write vectors and matrices to an ostream.
 */
namespace glm {
    /**
     * Creates a 4x4 matrix which performs a translation
     * \param dx - the x-component of the translation vector
     * \param dy - the y-component of the translation vector
     * \param dz - the z-component of the translation vector
     * \return a 4x4 matrix performing a translation
     */
    inline glm::mat4x4 translate(float dx, float dy, float dz){
        return glm::translate(glm::vec3(dx, dy, dz));
    }

    /**
     * Creates a 4x4 matrix which performs a scaling
     * \param s_x - scale factor in the x-direction
     * \param s_y - scale factor in the y-dirextion
     * \param s_z - scale factor in the z-dirextion
     * \return a 4x4 matrix performing a scaling
     */
    inline glm::mat4x4 scale(float s_x, float s_y, float s_z){
        return scale(glm::vec3(s_x, s_y, s_z));
    }

    /**
     * Create a 4x4 matrix which performs a rotation around the x-axis
     * \param angle - the rotation angle
     * \return a 4x4 matrix performing a translation
     */
    inline glm::mat4x4 rotateX(float angle){
        return glm::rotate(angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    /**
     * Create a 4x4 matrix which performs a rotation around the y-axis
     * \param angle - the rotation angle
     * \return a 4x4 matrix performing a translation
     */
    inline glm::mat4x4 rotateY(float angle){
        return glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    /**
     * Create a 4x4 matrix which performs a rotation around the z-axis
     * \param angle - the rotation angle
     * \return a 4x4 matrix performing a translation
     */
    inline glm::mat4x4 rotateZ(float angle){
        return glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    /**
     * Creates a 4x4 matrix which performs an XY-shear
     * \param sh_x - the shear factor in the x-direction
     * \param sh_y - the shear factor in the y-direction
     * \return a 4x4 matrix performing an XY shear
     */
    inline glm::mat4x4 shearXY(float sh_x, float sh_y){
        glm::vec4 shearparam(sh_x, sh_y, 1.0f, 0.0f);
        return glm::column(glm::mat4x4(1.0f), 2, shearparam);
    }

    /**
     * Creates a 4x4 matrix which performs an XY-shear
     * \param shearfactors - a vector containing the two shear factors
     * \return a 4x4 matrix performing an XY shear
     */
    inline glm::mat4x4 shearXY(glm::vec2 const& shearfactors){
        return shearXY(shearfactors[0], shearfactors[1]);
    }

    /**
     * Creates a 4x4 matrix which performs an XZ-shear
     * \param sh_x - the shear factor in the x-direction
     * \param sh_z - the shear factor in the z-direction
     * \return a 4x4 matrix performing an XZ shear
     */
    inline glm::mat4x4 shearXZ(float sh_x, float sh_z){
        glm::vec4 shearparam(sh_x, 1.0f, sh_z, 0.0f);
        return glm::column(glm::mat4x4(1.0f), 1, shearparam);
    }

    /**
     * Creates a 4x4 matrix which performs an XZ-shear
     * \param shearfactors - a vector containing the two shear factors
     * \return a 4x4 matrix performing an XZ shear
     */
    inline glm::mat4x4 shearXZ(glm::vec2 const& shearfactors){
        return shearXZ(shearfactors[0], shearfactors[1]);
    }

    /**
     * Creates a 4x4 matrix which performs an YZ-shear
     * \param sh_y - the shear factor in the y-direction
     * \param sh_z - the shear factor in the z-direction
     * \return a 4x4 matrix performing an YZ shear
     */
    inline glm::mat4x4 shearYZ(float sh_y, float sh_z){
        glm::vec4 shearparam(1.0f, sh_y, sh_z, 0.0f);
        return glm::column(glm::mat4x4(1.0f), 0, shearparam);
    }

    /**
     * Creates a 4x4 matrix which performs an YZ-shear
     * \param shearfactors - a vector containing the two shear factors
     * \return a 4x4 matrix performing an YZ shear
     */
    inline glm::mat4x4 shearYZ(glm::vec2 const& shearfactors){
        return shearYZ(shearfactors[0], shearfactors[1]);
    }

    /**
     * writes a glm::vec2 to a std::ostream
     * \param s - a std::ostream where the output should be written.
     * \param v - a glm::vec2 which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::vec2 const& v){
        s << ' ';
        for (int i = 0; i < 2; ++i) {
            s << std::setw(10) << std::setprecision(4) << v[i] << ' ';
        }
        return s;
    }
    
    /**
     * writes a glm::vec3 to a std::ostream
     * \param s - a std::ostream where the output should be written.
     * \param v - a glm::vec3 which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::vec3 const& v){
        s << ' ';
        for (int i = 0; i < 3; ++i) {
            s << std::setw(10) << std::setprecision(4) << v[i] << ' ';
        }
        return s;
    }

    /**
     * writes a glm::vec4 to a std::ostream
     * \param s - a std::ostream& where the output should be written.
     * \param v - a glm::vec4& which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::vec4 const& v){
        for (int i = 0; i < 4; ++i) {
            s << std::setw(10) << std::setprecision(4) << v[i] << ' ';
        }
        return s;
    }

    /**
     * writes a glm::mat2x2 to a std::ostream
     * \param s - a std::ostream where the output should be written.
     * \param m - a glm::mat2x2 which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::mat2x2 const& m){
        for (int r = 0; r < 2; ++r) {
            s << ' ';
            for (int c = 0; c < 2; ++c) {
                s << std::setw(10) << std::setprecision(4) << m[c][r] << ' ' << '\t';
            }
            s << std::endl;
        }
        return s;
    }

    /**
     * writes a glm::mat3x3 to a std::ostream
     * \param s - a std::ostream where the output should be written.
     * \param m - a glm::mat3x3 which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::mat3x3 const& m){
        for (int r = 0; r < 3; ++r) {
            s << ' ';
            for (int c = 0; c < 3; ++c) {
                s << std::setw(10) << std::setprecision(4) << m[c][r] << ' ' << '\t';
            }
            s << std::endl;
        }
        return s;
    }

    /**
     * writes a glm::mat4x4 to a std::ostream
     * \param s - a std::ostream where the output should be written.
     * \param m - a glm::mat4x4 which should be written.
     * \return the parameter s.
     */
    inline std::ostream& operator<<(std::ostream& s, glm::mat4x4 const& m){
        for (int r = 0; r < 4; ++r) {
            s << ' ';
            for (int c = 0; c < 4; ++c) {
                s << std::setw(10) << std::setprecision(4) << m[c][r] << ' ' << '\t';
            }
            s << std::endl;
        }
        return s;
    }
}

#endif
