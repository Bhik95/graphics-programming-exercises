//
// Created by Henrique Galvan Debarba on 2019-09-27.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_OGLFRAMEBUFFER_H
#define GRAPHICSPROGRAMMINGEXERCISES_OGLFRAMEBUFFER_H

#include <vector>

namespace srl {


    template<class T>
    class FrameBuffer {
    public:

        FrameBuffer(unsigned int width, unsigned int height);
        FrameBuffer(const FrameBuffer<T> &fb);
        ~FrameBuffer();

        inline unsigned int width() const { return m_width; }
        inline unsigned int height() const { return m_height; }
        inline unsigned int size() const { return m_size; }
        // we need to be able to get a pointer to the buffer to set the render texture
        inline T *buffer() const { return m_buffer; }
		
		inline unsigned int indexAt(unsigned int u, unsigned int v) const {
			return (v >= m_height ? m_height - 1 : v) * m_width + (u >= m_width ? m_width - 1 : u);
		}

		inline unsigned int indexAtNorm(float uNorm, float vNorm) const {
			unsigned int u = (uNorm * m_width);
			unsigned int v = (vNorm * m_height);
			return (v >= m_height ? m_height - 1 : v) * m_width + (u >= m_width ? m_width - 1 : u);
		}

        inline T &operator[](unsigned int index) { return m_buffer[index]; }

        FrameBuffer<T> &operator=(const FrameBuffer<T> &);

        // set frame buffer to value
        void clearBuffer(const T &value);
        // set frame buffer to zero
        void clearBuffer();

    private:

        unsigned int m_width;
        unsigned int m_height;
        unsigned int m_size;

        T *m_buffer;

    };

    // constructor
    template<class T>
    FrameBuffer<T>::FrameBuffer(unsigned int width, unsigned int height) {
        m_width = width;
        m_height = height;
        m_size = m_width * m_height;
        m_buffer = new T[m_size]; // memory allocation in C++
    }

    // copy constructor
    template<class T>
    FrameBuffer<T>::FrameBuffer(const FrameBuffer<T> &fb) {
        m_width = fb.m_width;
        m_height = fb.m_height;
        m_size = fb.m_size;
        m_buffer = new T[m_size]; // memory allocation in C++
        // make a copy of the buffer into the new object
        memcpy(m_buffer, fb.buffer(), sizeof(T) * m_size);
    }

    template<class T>
    FrameBuffer<T> &FrameBuffer<T>::operator=(const FrameBuffer<T> &fb) {
        delete[] m_buffer;
        m_width = fb.m_width;
        m_height = fb.m_height;
        m_size = fb.my_size;
        m_buffer = new T[m_size]; // memory allocation in C++
        // make a copy of the buffer into this object
        memcpy(m_buffer, fb.buffer(), sizeof(T) * m_size);
        return *this;
    }

    // constructor, necessary to release memory in C++
    template<class T>
    FrameBuffer<T>::~FrameBuffer() {
        delete[] m_buffer;
    }

    // set frame buffer value
    template<class T>
    void FrameBuffer<T>::clearBuffer(const T &value) {
        for (unsigned int i = 0; i < m_size; i++)
            m_buffer[i] = value;
    }

    // set frame buffer to zero
    template<class T>
    void FrameBuffer<T>::clearBuffer() {
        T value = 0;
        for (unsigned int i = 0; i < m_size; i++)
            m_buffer[i] = value;
    }

}


#endif //GRAPHICSPROGRAMMINGEXERCISES_OGLFRAMEBUFFER_H
