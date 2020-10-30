#ifndef __EDGE_H__
#define __EDGE_H__

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "software_renderer_lib/srl_types.h"

/**
 * \class edge_rasterizer
 * A class which scanconverts an edge in a polygon. It computes the pixels which
 * are closest to the ideal edge, i.e.e either on the edge or to the right of the edge.
 */ 
class edge_rasterizer {
public:
    /**
     * Default constructor creates an empty edge_rassterizer
     */
    edge_rasterizer();

    /**
     * Destructor destroys the edge_rasterizer
     */
    virtual ~edge_rasterizer();

    /**
     * Initializes the edge_rasterizer with one edge
     */
    void init(srl::vertex v1, srl::vertex v2);
    
    /**
     * Initializes the edge_rasterizer with two edges
     */
    void init(srl::vertex v1, srl::vertex v2, srl::vertex v3);

    /**
     * Checks if there are fragments/pixels on the edge ready for use
     * \return - true if there is a fragment/pixel on the edge ready for use, else it returns false
     */
    bool more_fragments() const;

    /**
     * Computes the next fragment/pixel on the edge
     */
    void next_fragment();

    /**
     * Returns the current x-coordinate of the current fragment/pixel on the edge
     * It is only valid to call this function if "more_fragments()" returns true,
     * else a "runtime_error" exception is thrown
     * \return - The x-coordinate of the current edge fragment/pixel
     */
    int x() const;

    /**
     * Returns the current x-coordinate of the current fragment/pixel on the edge
     * It is only valid to call this function if "more_fragments()" returns true,
     * else a "runtime_error" exception is thrown
     * \return - The y-coordinate of the current edge fragment/pixel
     */
    int y() const;

    srl::vertex getCurrent();

private:
    /**
     * Initializes an edge, so it is ready to be scanconverted
     * \return - true if slope of the edge != 0 , false if the edge is horizontal
     */
    bool init_edge(srl::vertex v1, srl::vertex v2);

    /**
     * Computes the next fragment/pixel on the edge
     */
    void update_edge();

    /**
     * This variable is true if the edge_rasterizer scanconverts two edges
     */
    bool two_edges;

    /**
     * The coordinates of the current start point, stop point, and the current point
     */
    int x_start;   int y_start;
    int x_stop;    int y_stop;
    int x_current; int y_current;

    /**
     * valid is true of there are a point available on the edge
     */
    bool valid;

    /**
     * The differences in x- and y-coordinates
     */
    int dx; int dy;

    /**
     * The step sizes of the x- and y-coordinates
     */
    int x_step; int y_step;

    /**
     * Variables used to determine the next pixels coordinates
     */
    int Numerator;
    int Denominator;
    int Accumulator;

    srl::vertex m_v1, m_v2, m_v3;

    // TODO
    // use these variables for interpolation
    srl::vertex m_current;
    srl::vertex m_step;


};

#endif
