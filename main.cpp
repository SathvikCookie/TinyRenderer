#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include "our_gl.h"
#include "model.h"

extern mat<4,4> ModelView, Perspective;
extern std::vector<double> zbuffer;

struct PhongShader : IShader {
    const Model &model;
    vec3 l;
    vec3 tri[3];

    PhongShader(const vec3 light, const Model &m) : model(m) {
        l = normalized((ModelView * vec4{light.x, light.y, light.z, 0.0}).xyz());
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        tri[vert] = gl_Position.xyz();
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        TGAColor gl_FragColor = {255, 255, 255, 255};     
        vec3 n = normalized(cross(tri[1]-tri[0], tri[2]-tri[0]));
        vec3 r = normalized(2*n*(n*l) - l);
        double ambient = 0.3;
        double diff = std::max(0., n*l);
        double spec = std::pow(std::max(0., r.z), 35);
        for (int c : {0,1,2}) {
            gl_FragColor[c] *= std::min(1., ambient + .4*diff + .9*spec);
        }
        return {false, gl_FragColor};
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr int width  = 800;
    constexpr int height = 800;
    constexpr vec3 light{ 1, 1, 1};
    constexpr vec3 eye{-1,0,2};
    constexpr vec3 center{0,0,0};
    constexpr vec3 up{0,1,0};

    lookat(eye, center, up);
    init_perspective(norm(eye-center));
    init_viewport(width/16, height/16, width*7/8, height*7/8);
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);
        PhongShader shader(light, model);

        for (int f=0; f<model.nfaces(); f++) {
            Triangle clip = {
                shader.vertex(f, 0),
                shader.vertex(f, 1),
                shader.vertex(f, 2)
            };
            rasterize(clip, shader, framebuffer);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
