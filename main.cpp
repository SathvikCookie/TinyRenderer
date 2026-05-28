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
    vec4 l;
    vec2 varying_uv[3];
    vec4 varying_norm[3];
    vec4 tri[3];

    PhongShader(const vec3 light, const Model &m) : model(m) {
        l = normalized(ModelView * vec4{light.x, light.y, light.z, 0.0});
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        
        varying_uv[vert] = model.uv(face, vert);
        varying_norm[vert] = model.normal(face, vert);
        tri[vert] = gl_Position;
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec2 uv = varying_uv[0]*bar[0] + varying_uv[1]*bar[1] + varying_uv[2]*bar[2];

        // Calculate tangent basis
        mat<2,4> E = {{
            {tri[1] - tri[0]}, // e0
            {tri[2] - tri[0]}  // e1
        }};

        mat<2,2> U = {{
            {varying_uv[1] - varying_uv[0]}, // u0
            {varying_uv[2] - varying_uv[0]}  // u1
        }};

        mat<2,4> TB = U.invert() * E;

        mat<4,4> TBN = {{
            {normalized(TB[0])},
            {normalized(TB[1])},
            {normalized(varying_norm[0]*bar[0] + varying_norm[1]*bar[1] + varying_norm[2]*bar[2])},
            {0,0,0,1}
        }};

        vec4 n = normalized(TBN.transpose() * model.normal(uv));

        vec4 r = normalized(2*n*(n*l) - l);
        double ambient = .4;
        double diffuse = 1.*std::max(0., n*l);
        double specular = (3.*sample2D(model.specular(), uv)[0]/255.) * std::pow(std::max(r.z, 0.), 35);

        TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
        for (int c : {0,1,2}) {
            gl_FragColor[c] = std::min<int>(255, gl_FragColor[c]*(ambient + diffuse + specular));
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
    constexpr vec3 light{1, 1, 1};
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
