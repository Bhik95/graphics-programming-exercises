#include "edgerasterizer.h"


/*
 * \class edge_rasterizer
 * A class which scanconverts an edge in a polygon. It computes the pixels which
 * are closest to the ideal edge, i.e.e either on the edge or to the right of the edge.
 */


/*
 * Default constructor creates an empty edge_rassterizer
 */
edge_rasterizer::edge_rasterizer() : valid(false)
{}

/*
 * Destructor destroys the edge_rasterizer
 */
edge_rasterizer::~edge_rasterizer()
{}

/*
 * Initializes the edge_rasterizer with one edge
 */
void edge_rasterizer::init(srl::vertex v1, srl::vertex v2)
{
    m_v1 = v1;
    m_v2 = v2;

    this->two_edges = false;
    this->init_edge(m_v1, m_v2);
}

/*
 * Initializes the edge_rasterizer with two edges
 */
void edge_rasterizer::init(srl::vertex v1, srl::vertex v2, srl::vertex v3)
{
    m_v1 = v1;
    m_v2 = v2;
    m_v3 = v3;

    this->two_edges = true;

    bool horizontal = !(this->init_edge(m_v1, m_v2));
    if (horizontal) { // edge 1 is horizontal
        this->two_edges = false;
        this->init_edge(m_v2, m_v3);
    }
}

/*
 * Checks if there are fragments/pixels on the edge ready for use
 * \return - true if there is a fragment/pixel on the edge ready for use, else it returns false
 */
bool edge_rasterizer::more_fragments() const
{
    return this->valid;
}

/*
 * Computes the next fragment/pixel on the edge
 */
void edge_rasterizer::next_fragment()
{
    // TODO
    // interpolate along the line here ~ one line of code



    this->y_current += this->y_step;

    if (this->y_current < this->y_stop)
        this->update_edge();
    else {
        if (this->two_edges) {
            this->init_edge(m_v2, m_v3);
            this->two_edges = false;
        }
    }
    this->valid = (this->y_current < this->y_stop);
}

/*
 * Returns the current x-coordinate of the current fragment/pixel on the edge
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return - The x-coordinate of the current edge fragment/pixel
 */
int edge_rasterizer::x() const
{
    if (!this->valid) {
        throw std::runtime_error("edge_rasterizer::x(): Invalid State");
    }
    return this->x_current;
}

/*
 * Returns the current x-coordinate of the current fragment/pixel on the edge
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return - The y-coordinate of the current edge fragment/pixel
 */
int edge_rasterizer::y() const
{
    if (!this->valid) {
        throw std::runtime_error("edge_rasterizer::y(): Invalid State");
    }
    return this->y_current;
}


srl::vertex edge_rasterizer::getCurrent(){
    return m_current;
}


/*
 * Initializes an edge, so it is ready to be scanconverted
 * \return - true if slope of the edge != 0 , false if the edge is horizontal
 */
bool edge_rasterizer::init_edge(srl::vertex v1, srl::vertex v2)
{
    this->x_start = int(v1.pos.x + 0.5f); this->y_start = int(v1.pos.y + 0.5f);
    this->x_stop  = int(v2.pos.x + 0.5f); this->y_stop  = int(v2.pos.y + 0.5f);
    this->x_current = this->x_start; this->y_current = this->y_start;

    int dx = this->x_stop - this->x_start;
    int dy = this->y_stop - this->y_start;

    this->x_step = (dx < 0) ? -1 : 1;
    this->y_step = 1;

    this->Numerator   = std::abs(dx); // Numerator = |dx|
    this->Denominator = std::abs(dy); // Assumption: dy > 0
    this->Accumulator = (x_step > 0) ? Denominator : 1;

    this->valid = (this->y_current < this->y_stop);


    // TODO
    // initialize the vertex variables used for interpolation ~ 3 lines (I deleted m_start and reduced to 2 lines)


    return this->valid;
}

/*
 * Computes the next fragment/pixel on the edge
 */
void edge_rasterizer::update_edge()
{
    this->Accumulator += this->Numerator;
    while (this->Accumulator > this->Denominator) {
        this->x_current   += this->x_step;
        this->Accumulator -= this->Denominator;
    }
}
