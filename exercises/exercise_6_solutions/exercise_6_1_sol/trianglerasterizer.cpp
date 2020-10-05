#include "trianglerasterizer.h"

/*
 * \class triangle_rasterizer
 * A class which scanconverts a triangle. It computes the pixels such that they are inside the triangle.
 */
triangle_rasterizer::triangle_rasterizer(int x1, int y1, int x2, int y2, int x3, int y3) : valid(false)
{
    this->initialize_triangle(x1, y1, x2, y2, x3, y3);
}

/*
 * Destroys the current instance of the triangle rasterizer
 */
triangle_rasterizer::~triangle_rasterizer()
{}

/*
 * Returns a vector which contains alle the pixels inside the triangle
 */
std::vector<glm::ivec2> triangle_rasterizer::all_pixels()
{
    std::vector<glm::ivec2> points;

    // TODO
    while (this->more_fragments()) {
        points.push_back(glm::ivec2(x_current, y_current));
        this->next_fragment();
    }

    return points;
}

/*
 * Checks if there are fragments/pixels inside the triangle ready for use
 * \return true if there are more fragments in the triangle, else false is returned
 */
bool triangle_rasterizer::more_fragments() const
{
    return this->valid;
}

/*
 * Computes the next fragment inside the triangle
 */
void triangle_rasterizer::next_fragment()
{
    // TODO
    if (this->x_current < this->x_stop) {
        this->x_current += 1;
    }
    else {
        this->leftedge.next_fragment();
        this->rightedge.next_fragment();
        while (this->leftedge.more_fragments() && (leftedge.x() >= rightedge.x())) {
            leftedge.next_fragment();
            rightedge.next_fragment();
        }
        this->valid = this->leftedge.more_fragments();
        if (this->valid) {
            this->x_start   = leftedge.x();
            this->x_current = this->x_start;
            this->x_stop    = rightedge.x() - 1;
            this->y_current = leftedge.y();
        }
    }
}

/*
 * Returns the current x-coordinate of the current fragment/pixel inside the triangle
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return The x-coordinate of the current triangle fragment/pixel
 */
int triangle_rasterizer::x() const
{
    if (!this->valid) {
        throw std::runtime_error("triangle_rasterizer::x(): Invalid State/Not Initialized");
    }
    return this->x_current;
}

/*
 * Returns the current y-coordinate of the current fragment/pixel inside the triangle
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return The y-coordinate of the current triangle fragment/pixel
 */
int triangle_rasterizer::y() const
{
    if (!this->valid) {
        throw std::runtime_error("triangle_rasterizer::y(): Invalid State/Not Initialized");
    }
    return this->y_current;
}

/*
 * Initializes the TriangleRasterizer with the three vertices
 * \param x1 - the x-coordinate of the first vertex
 * \param y1 - the y-coordinate of the first vertex
 * \param x2 - the x-coordinate of the second vertex
 * \param y2 - the y-coordinate of the second vertex
 * \param x3 - the x-coordinate of the third vertex
 * \param y3 - the y-coordinate of the third vertex
 */
void triangle_rasterizer::initialize_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    // TODO
    this->ivertex[0] = glm::ivec2(x1, y1);
    this->ivertex[1] = glm::ivec2(x2, y2);
    this->ivertex[2] = glm::ivec2(x3, y3);

    this->lower_left = this->LowerLeft();
    this->upper_left = this->UpperLeft();
    this->the_other  = 3 - lower_left - upper_left;

    glm::ivec2 ll = this->ivertex[this->lower_left];
    glm::ivec2 ul = this->ivertex[this->upper_left];
    glm::ivec2 ot = this->ivertex[this->the_other];

    // Let u be the vector from 'lower_left' to 'upper_left' vertices.
    glm::ivec2 u(ul - ll);

    // Let v be the vector from 'lower_left' to 'the_other'.
    glm::ivec2 v(ot - ll);

    // If the cross product (u x v) has a positive
    // z-component then the point 'the_other' is to the left of u, else it is to the
    // right of u.
    int z_component_of_the_cross_product = u.x * v.y- u.y * v.x;

    if (z_component_of_the_cross_product != 0) {
        if (z_component_of_the_cross_product > 0) {
            // The vertex the_other is to the left of the longest vector u.
            // Therefore, the leftedge has two edges associated to it
            // (lower_left -> the_other), and (the_other -> upper_left),
            // while the right edge has only one (lower_left -> upper_left).
            this->leftedge.init(ll.x, ll.y, ot.x, ot.y, ul.x, ul.y);
            this->rightedge.init(ll.x, ll.y, ul.x, ul.y);
        }
        else {
            // The vertex the_other is to the right of the longest vector u.
            // Therefore, the leftedge has only one edge assigned to it
            // (lower_left -> upper_left), while the  rightedge has two edges
            // associated to it (lower_left -> the_other), and (the_other -> upper_left).
            this->leftedge.init(ll.x, ll.y, ul.x, ul.y);
            this->rightedge.init(ll.x, ll.y, ot.x, ot.y, ul.x, ul.y);
        }

        // Now the leftedge and rightedge `edge_rasterizers' are initialized, so they are
        // ready for use.

        this->x_start   = this->leftedge.x();
        this->y_start   = this->leftedge.y();

        this->x_current = this->x_start;
        this->y_current = this->y_start;

        this->x_stop    = this->rightedge.x() - 1;
        this->y_stop    = this->ivertex[this->upper_left].y;

        this->valid = (this->x_current <= this->x_stop);
        if (!(this->valid)) {
            this->next_fragment();
        }
    }
}

/*
 * Computes the index of the lower left vertex in the array ivertex
 * \return the index in the vertex table of the lower left vertex
 */
int triangle_rasterizer::LowerLeft()
{
    int ll = 0;
    // TODO
    for (int i = ll + 1; i < 3; ++i) {
        if (this->ivertex[i].y < this->ivertex[ll].y ||
            (this->ivertex[i].y == this->ivertex[ll].y && this->ivertex[i].x < this->ivertex[ll].x)){
            ll = i;
        }
    }

    return ll;
}

/*
 * Computes the index of the upper left vertex in the array ivertex
 * \return the index in the vertex table of the upper left vertex
 */
int triangle_rasterizer::UpperLeft()
{
    int ul = 0;
    // TODO
    for (int i = ul + 1; i < 3; ++i) {
        if (this->ivertex[i].y > this->ivertex[ul].y ||
            (this->ivertex[i].y == this->ivertex[ul].y && this->ivertex[i].x < this->ivertex[ul].x)){
            ul = i;
        }
    }

    return ul;
}
