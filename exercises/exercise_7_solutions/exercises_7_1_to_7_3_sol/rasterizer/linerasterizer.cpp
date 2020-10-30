#include "rasterizer/linerasterizer.h"


/*
 * \class LineRasterizer
 * A class which scanconverts a straight line. It computes the pixels such that they are as close to the
 * the ideal line as possible.
 */ 

/*
 * Parameterized constructor creates an instance of a line rasterizer
 * \param x1 - the x-coordinate of the first vertex
 * \param y1 - the y-coordinate of the first vertex
 * \param x2 - the x-coordinate of the second vertex
 * \param y2 - the y-coordinate of the second vertex
 */
/*LineRasterizer::LineRasterizer(int x1, int y1, int x2, int y2)
{
    this->initialize_line(x1, y1, x2, y2);
}*/
LineRasterizer::LineRasterizer(srl::vertex v1, srl::vertex v2){
    m_start=v1;
    m_stop=v2;
    this->initialize_line();
}

/*
 * Destroys the current instance of the line rasterizer
 */
LineRasterizer::~LineRasterizer()
{}


/*
 * Checks if there are fragments/pixels of the line ready for use
 * \return true if there are more fragments of the line, else false is returned
 */
bool LineRasterizer::MoreFragments() const
{
    return this->valid;
}

/*
 * Computes the next fragment of the line
 */
void LineRasterizer::NextFragment()
{
    // Run the innerloop once
    // Dereference the pointer to the private member function 
    // It looks strange; but this is the way it is done!
    (this->*innerloop)();
}

/*
 * Returns a vector which contains all the pixels of the line
 */
std::vector<glm::ivec2> LineRasterizer::AllFragments(){
    std::vector<glm::ivec2> points;

    while (this->MoreFragments()) {
        points.push_back(glm::ivec2(this->x(), this->y()));
        this->NextFragment();
    }

    return points;
}


/*
 * Returns the current x-coordinate of the current fragment/pixel of the line
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return The x-coordinate of the current line fragment/pixel
 */
int LineRasterizer::x() const
{
    if (!this->valid) {
	    throw std::runtime_error("LineRasterizer::x(): Invalid State");
    }
    return this->x_current;
}

/*
 * Returns the current y-coordinate of the current fragment/pixel of the line
 * It is only valid to call this function if "more_fragments()" returns true,
 * else a "runtime_error" exception is thrown
 * \return The y-coordinate of the current line fragment/pixel
 */
int LineRasterizer::y() const
{
    if (!this->valid) {
	    throw std::runtime_error("LineRasterizer::y(): Invalid State");
    }
    return this->y_current;
}

srl::vertex LineRasterizer::GetCurrent()
{
    return this->m_current;
}

/*
 * Protected functions
 */

/*
 * Private functions
 */

/*
 * Initializes the LineRasterizer with the two vertices
 */
void LineRasterizer::initialize_line()
{
    this->x_start = m_start.pos.x + 0.5f;
    this->y_start = m_start.pos.y + 0.5f;

    this->x_stop = m_stop.pos.x + 0.5f;
    this->y_stop = m_stop.pos.y + 0.5f;

    this->x_current = this->x_start;
    this->y_current = this->y_start;

    this->dx = this->x_stop - this->x_start;
    this->dy = this->y_stop - this->y_start;

    this->abs_2dx = std::abs(this->dx) << 1; // 2 * |dx|
    this->abs_2dy = std::abs(this->dy) << 1; // 2 * |dy|

    this->x_step = (this->dx < 0) ? -1 : 1;
    this->y_step = (this->dy < 0) ? -1 : 1;

    m_current = m_start;

    if (this->abs_2dx > this->abs_2dy) {
        // the line is x-dominant
        this->left_right = (this->x_step > 0);
        this->d = this->abs_2dy - (this->abs_2dx >> 1);
        this->valid = (this->x_start != this->x_stop);
        this->innerloop = &LineRasterizer::x_dominant_innerloop;

        m_step = (m_stop - m_start) / float(std::abs(this->dx));
    }
    else {
        // the line is y-dominant
        this->left_right = (this->y_step > 0);
        this->d = this->abs_2dx - (this->abs_2dy >> 1);
        this->valid = (this->y_start != this->y_stop);
        this->innerloop = &LineRasterizer::y_dominant_innerloop;

        m_step = (m_stop - m_start) / float(std::abs(this->dy));
    }
}

/*
 * Runs the x-dominant innerloop
 */
void LineRasterizer::x_dominant_innerloop()
{
    if ((this->valid = (this->x_current != this->x_stop))) {
	    if (this->d > 0 || (this->d == 0 && this->left_right)) {
            this->y_current += this->y_step;
            this->d         -= this->abs_2dx;
        }
        this->x_current += this->x_step;
        this->d         += this->abs_2dy;

        m_current = m_current + m_step;
    }
}

/*
 * Runs the y-dominant innerloop
 */
void LineRasterizer::y_dominant_innerloop()
{
    if ((this->valid = (this->y_current != this->y_stop))) {
        if (this->d > 0 || (this->d == 0 && this->left_right)) {
            this->x_current += this->x_step;
            this->d         -= this->abs_2dy;
        }
        this->y_current += this->y_step;
        this->d         += this->abs_2dx;

        m_current = m_current + m_step;
    }
}
