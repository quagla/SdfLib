#include "RealSdf.h"

RealSdf::RealSdf(Mesh& mesh)
{
    std::vector<uint32_t>& indices = mesh.getIndices();
    std::vector<glm::vec3>& vertices = mesh.getVertices();
    mTriangles = std::move(TriangleUtils::calculateMeshTriangleData(mesh));
}

float RealSdf::getDistance(glm::vec3 sample) const
{
    float minDist = INFINITY;
	uint32_t nearestTriangle = 0;
    for(uint32_t t=0; t < mTriangles.size(); t++)
    {
        const float dist = TriangleUtils::getSqDistPointAndTriangle(sample, mTriangles[t]);
		if (dist < minDist)
		{
			nearestTriangle = t;
			minDist = dist;
		}
    }

    return TriangleUtils::getSignedDistPointAndTriangle(sample, mTriangles[nearestTriangle]);
}