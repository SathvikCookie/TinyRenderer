#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include "geometry.h"
#include "tgaimage.h"
#include "model.h"

constexpr int width  = 800;
constexpr int height = 800;
constexpr vec3 eye{-1,0,2};
constexpr vec3 center{0,0,0};
constexpr vec3 up{0,1,0};

mat<4,4> ModelView, ViewPort, Perspective;

void viewport(const int x, const int y, const int w, const int h) {
    ViewPort = {{
        {w/2., 0, 0, x+w/2.},
        {0, h/2., 0, y+h/2.},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
}

void perspective(const double f) {
    Perspective = {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, -1/f, 1}
    }};
}

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalized(eye-center);
    vec3 l = normalized(cross(up, n));
    vec3 m = normalized(cross(n, l));
    ModelView = mat<4,4>{{
        {l.x, l.y, l.z, 0},
        {m.x, m.y, m.z, 0},
        {n.x, n.y, n.z, 0},
        {0, 0, 0, 1}
    }} * mat<4,4>{{
        {1, 0, 0, -center.x},
        {0, 1, 0, -center.y},
        {0, 0, 1, -center.z},
        {0, 0, 0, 1}
    }};
}

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

void rasterize(vec4 clip[3], TGAImage &framebuffer, std::vector<double> &zbuffer, TGAColor color) {
    vec4 ndc[3] = {clip[0]/clip[0].w, clip[1]/clip[1].w, clip[2]/clip[2].w};
    vec2 screen[3] = {(ViewPort * ndc[0]).xy(), (ViewPort * ndc[1]).xy(), (ViewPort * ndc[2]).xy()};

    mat<3,3> ABC = {{
        {screen[0].x, screen[0].y, 1.},
        {screen[1].x, screen[1].y, 1.},
        {screen[2].x, screen[2].y, 1.},
    }};

    if (ABC.det() < 1) {
        return;
    }

    auto [minx, maxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [miny, maxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y});

    #pragma omp parallel for
    for (int x = minx; x <= maxx; x++) {
        for (int y = miny; y <= maxy; y++) {
            vec3 bc = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1.};
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) {
                continue;
            }
            double z = bc * vec3{ndc[0].z, ndc[1].z, ndc[2].z};

            if (z <= zbuffer[x+y*framebuffer.width()]) {
                continue;
            }

            zbuffer[x+y*framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width*height, -std::numeric_limits<double>::max());

    viewport(width/16, height/16, width*7/8, height*7/8);
    perspective(norm(eye-center));
    lookat(eye, center, up);

    for (int m = 1; m < argc; m++) {
        Model model(argv[1]);

        for (int i=0; i<model.nfaces(); i++) {
            vec4 clip[3];

            for (int d : {0, 1, 2}) {
                vec3 v = model.vert(i, d);
                clip[d] = Perspective * ModelView * vec4{v.x, v.y, v.z, 1};
            }

            TGAColor rnd;
            for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
            rasterize(clip, framebuffer, zbuffer, rnd);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
