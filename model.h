#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
    std::vector<vec4> verts = {}; // Vector to store the coordinates of each vertex in the point cloud
    std::vector<vec4> norms = {}; // Vector to store the normals of each vertex in the point cloud
    std::vector<vec2> tex = {};

    std::vector<int> facet_vrt = {}; // Vector to store the index of the vertices mapped to each face, grouped in multiples of 3
    std::vector<int> facet_norm = {}; // Vector to store the index of the normals mapped to each face, grouped in multiples of 3
    std::vector<int> facet_tex = {};

    TGAImage normalmap = {};
    TGAImage diffusemap = {};
    TGAImage specularmap = {};

    public:
        Model(const std::string filename);
        int nverts() const;
        int nfaces() const;
        vec4 vert(const int i) const; // Directly get the i'th vertice from the point cloud
        vec4 vert(const int iface, int nthvert) const; // Get the n'th vertex of the i'th face of the object
        vec4 normal(const int iface, int nthnorm) const; // Get the n'th norm of the i'th face of the object
        vec4 normal(const vec2 &uv) const;
        vec2 uv(const int iface, const int nthvert) const;
        const TGAImage& diffuse() const;
        const TGAImage& specular() const;
};