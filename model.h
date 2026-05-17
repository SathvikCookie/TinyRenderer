#include <vector>
#include "geometry.h"

class Model {
    std::vector<vec3> verts = {}; // Vector to store the coordinates of each vertex in the point cloud
    std::vector<int> facet_vrt = {}; // Vector to store the index of the vertices mapped to each face, grouped in multiples of 3

    public:
        Model(const std::string filename);
        int nverts() const;
        int nfaces() const;
        vec3 vert(const int i) const; // Directly get the i'th vertice from the point cloud
        vec3 vert(const int iface, int nthvert) const; // Get the n'th vertex of the i'th face of the object
};