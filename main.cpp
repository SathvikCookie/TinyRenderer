#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include "geometry.h"
#include "tgaimage.h"
#include "model.h"

constexpr int width  = 800;
constexpr int height = 800;

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);

    if (steep) { // Transpose if the line is steep
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax > bx) {  // Make sure the coordinates are left to right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++) {
        if (steep) { // If transposed, de-transpose
            framebuffer.set(y, x, color);
        } else {
            framebuffer.set(x, y, color);
        }

        ierror += 2 * std::abs(by - ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= (2 * (bx - ax)) * (ierror > bx - ax);
    }
}

vec3 rot(vec3 v) {
    double a = M_PI / 2;
    mat<3,3> Ry = {{
        {std::cos(a), 0, std::sin(a)}, 
        {0, 1, 0}, 
        {-std::sin(a), 0, std::cos(a)}
    }};
    return v*Ry;
}

vec3 persp(vec3 v) {
    double c = 3.;
    return v / (1 - v.z/c);
}

std::tuple<int, int, double> project(vec3 v) {
    return {
        (v.x + 1.0) * width/2,
        (v.y + 1.0) * height/2,
        (v.z)
    };
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return 0.5 * ((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

// Bounding Box Rasterization
void triangle(int ax, int ay, double az, int bx, int by, double bz, int cx, int cy, double cz, TGAImage &framebuffer, std::vector<double> &zbuffer, TGAColor color) {
    // Find min and max x and y coordinates
    int minX = std::max(0, std::min(ax, std::min(bx, cx)));
    int minY = std::max(0, std::min(ay, std::min(by, cy)));
    int maxX = std::min(width - 1, std::max(ax, std::max(bx, cx)));
    int maxY = std::min(height - 1, std::max(ay, std::max(by, cy)));

    // Calculate barycentric coordinates to determine if pixel is inside the triangle
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) {
        return;
    }

    #pragma omp parallel for
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(ax, ay, x, y, cx, cy) / total_area;
            double gamma = signed_triangle_area(ax, ay, bx, by, x, y) / total_area;

            // Any negative weight indicates the pixel is outside of the triangle
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            double z = alpha*az + beta*bz + gamma*cz;

            if (z <= zbuffer[x+y*width]) {
                continue;
            }

            zbuffer[x+y*width] = z;
            framebuffer.set(x, y, color);
        }
    }
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width*height, -std::numeric_limits<double>::max());
    Model model(argv[1]);

    for (int i=0; i<model.nfaces(); i++) {
        auto [ax, ay, az] = project(persp(rot(model.vert(i, 0))));
        auto [bx, by, bz] = project(persp(rot(model.vert(i, 1))));
        auto [cx, cy, cz] = project(persp(rot(model.vert(i, 2))));

        TGAColor rnd;
        for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
        triangle(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer, zbuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
