#include <vector>
#include <numbers>
#include <memory>
#include <glm/glm.hpp>
#include "Mesh.h"

// https://gist.github.com/Pikachuxxxx/5c4c490a7d7679824e0e18af42918efc

std::shared_ptr<Mesh> GenerateSphereSmooth(int radius, int latitudes, int longitudes, glm::vec3 color = glm::vec3 {1.f, 1.f, 1.f})
{
    if(longitudes < 3)
        longitudes = 3;
    if(latitudes < 2)
        latitudes = 2;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float pi = std::numbers::pi_v<float>;
    float nx, ny, nz, lengthInv = 1.0f / radius;    // normal

    float deltaLatitude = pi / latitudes;
    float deltaLongitude = 2 * pi / longitudes;
    float latitudeAngle;
    float longitudeAngle;

    // Compute all vertices first except normals
    for (int i = 0; i <= latitudes; ++i)
    {
        latitudeAngle = pi / 2 - i * deltaLatitude; /* Starting -pi/2 to pi/2 */
        float xy = radius * cosf(latitudeAngle);    /* r * cos(phi) */
        float z = radius * sinf(latitudeAngle);     /* r * sin(phi )*/

        /*
         * We add (latitudes + 1) vertices per longitude because of equator,
         * the North pole and South pole are not counted here, as they overlap.
         * The first and last vertices have same position and normal, but
         * different tex coords.
         */
        for (int j = 0; j <= longitudes; ++j)
        {
            longitudeAngle = j * deltaLongitude;

            auto x = xy * cosf(longitudeAngle);       /* x = r * cos(phi) * cos(theta)  */
            auto y = xy * sinf(longitudeAngle);       /* y = r * cos(phi) * sin(theta) */
            auto s = (float)j/longitudes;             /* s */
            auto t = (float)i/latitudes;              /* t */

            Vertex vertex;
            vertex.pos = glm::vec3(x, y, z);
            vertex.texCoord = glm::vec2(s, t);
            vertex.color = color;

            // normalized tVertex normal
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertex.normal = glm::vec3(nx, ny, nz);
            vertices.push_back(vertex);
        }
    }

    /*
     *  Indices
     *  k1--k1+1
     *  |  / |
     *  | /  |
     *  k2--k2+1
     */
    unsigned int k1, k2;
    for(int i = 0; i < latitudes; ++i)
    {
        k1 = i * (longitudes + 1);
        k2 = k1 + longitudes + 1;
        // 2 Triangles per latitude block excluding the first and last longitudes blocks
        for(int j = 0; j < longitudes; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (latitudes - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    auto mesh = std::make_shared<Mesh>();

    mesh->_vertices = vertices;
    mesh->_indices = indices;

    return mesh;
}