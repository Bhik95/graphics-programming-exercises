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
 * \param x1 - The x-coordinate of the lower edge point
 * \param y1 - The y-coorsinate of the lower edge point
 * \param x2 - The x-coordinate of the upper edge point
 * \param y2 - The y-coordinate of the upper edge point
 */
void edge_rasterizer::init(int x1, int y1, int x2, int y2)
{
    // TODO
    this->two_edges = false;
    this->x1 = x1; this->y1 = y1;
    this->x2 = x2; this->y2 = y2;
    this->init_edge(x1, y1, x2, y2);
}

/*
 * Initializes the edge_rasterizer with two edges
 * \param x1 - The x-coordinate of the lower point of edge one
 * \param y1 - The y-coorsinate of the lower point of edge one
 * \param x2 - The x-coordinate of the upper point of edge one = the lower point of edge two
 * \param y2 - The y-coordinate of the upper point of edge one = the lower point of edge two
 * \param x3 - The x-coordinate of the upper point of edge two
 * \param y3 - The y-coordinate of the upper point of edge two
 */
void edge_rasterizer::init(int x1, int y1, int x2, int y2, int x3, int y3)
{
    // TODO
    this->two_edges = true;
    this->x1 = x1; this->y1 = y1;
    this->x2 = x2; this->y2 = y2;
    this->x3 = x3; this->y3 = y3;

    bool horizontal = !(this->init_edge(x1, y1, x2, y2));
    if (horizontal) { // edge 1 is horizontal
        this->two_edges = false;
        this->init_edge(x2, y2, x3, y3);
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
    this->y_current += this->y_step;
    if (this->y_current < this->y_stop)
        this->update_edge();
    else {
        if (this->two_edges) {
            this->init_edge(x2, y2, x3, y3);
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

/*
 * Initializes an edge, so it is ready to be scanconverted
 * \param x1 - The x-coordinate of the lower edge point
 * \param y1 - The y-coorsinate of the lower edge point
 * \param x2 - The x-coordinate of the upper edge point
 * \param y2 - The y-coordinate of the upper edge point
 * \return - true if slope of the edge != 0 , false if the edge is horizontal
 */
bool edge_rasterizer::init_edge(int x1, int y1, int x2, int y2)
{
    // TODO
    this->x_start = x1; this->y_start = y1;
    this->x_stop  = x2; this->y_stop  = y2;
    this->x_current = this->x_start; this->y_current = this->y_start;

    int dx = this->x_stop - this->x_start;
    int dy = this->y_stop - this->y_start;

    this->x_step = (dx < 0) ? -1 : 1;
    this->y_step = 1;
    this->Numerator   = std::abs(dx); // Numerator = |dx|
    this->Denominator = std::abs(dy); // Assumption: dy > 0
    this->Accumulator = (x_step > 0) ? Denominator : 1;

    this->valid = (this->y_current < this->y_stop);

    return this->valid;
}

/*
 * Computes the next fragment/pixel on the edge
 */
void edge_rasterizer::update_edge()
{
    // TODO
    this->Accumulator += this->Numerator;
    while (this->Accumulator > this->Denominator) {
        this->x_current   += this->x_step;
        this->Accumulator -= this->Denominator;
    }
}
