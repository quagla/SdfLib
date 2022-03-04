#include "UniformGridSdf.h"
#include "utils/TriangleUtils.h"

#include <iostream>
#include <spdlog/spdlog.h>

UniformGridSdf::UniformGridSdf(Mesh& mesh, BoundingBox box, float cellSize)
    : mCellSize(cellSize)
{
    mGridSize = glm::ivec3(glm::ceil((box.max - box.min) / cellSize)) + glm::ivec3(1);
    SPDLOG_INFO("Uniform grid size: {}, {}, {}", mGridSize.x, mGridSize.y, mGridSize.z);

    mBox.min = box.min;
    mBox.max = box.min + mCellSize * glm::vec3(mGridSize - glm::ivec3(1));

    mGrid = std::vector<float>(mGridSize.x * mGridSize.y * mGridSize.z);
    mGridXY = mGridSize.x * mGridSize.y;

    std::vector<TriangleUtils::TriangleData> trianglesData(TriangleUtils::calculateMeshTriangleData(mesh));

    for(int z = 0; z < mGridSize.z; z++)
    {
        for(int y = 0; y < mGridSize.y; y++)
        {
            for(int x = 0; x < mGridSize.x; x++)
            {
                glm::vec3 cellPoint = mBox.min + glm::vec3(x, y, z) * mCellSize;

                float minDist = INFINITY;
				uint32_t nearestTriangle = 0;
                for(uint32_t t=0; t < trianglesData.size(); t++)
                {
                    const float dist = 
                        TriangleUtils::getSqDistPointAndTriangle(cellPoint, trianglesData[t]);
					if (dist < minDist)
					{
						nearestTriangle = t;
						minDist = dist;
					}
                }

                mGrid[z * mGridXY + y * mGridSize.x + x] = 
                    TriangleUtils::getSignedDistPointAndTriangle(cellPoint, trianglesData[nearestTriangle]);
            }
        }
    }
}

float UniformGridSdf::getDistance(glm::vec3 sample) const
{
    glm::vec3 fracPart = (sample - mBox.min) / mCellSize;
    glm::ivec3 arrayPos = glm::floor(fracPart);
    fracPart = glm::fract(fracPart);

    float d00 = mGrid[arrayPos.z * mGridXY + arrayPos.y * mGridSize.x + arrayPos.x] * (1.0f - fracPart.x) +
                mGrid[arrayPos.z * mGridXY + arrayPos.y * mGridSize.x + arrayPos.x + 1] * fracPart.x;
    float d01 = mGrid[arrayPos.z * mGridXY + (arrayPos.y + 1) * mGridSize.x + arrayPos.x] * (1.0f - fracPart.x) +
                mGrid[arrayPos.z * mGridXY + (arrayPos.y + 1) * mGridSize.x + arrayPos.x + 1] * fracPart.x;
    float d10 = mGrid[(arrayPos.z + 1) * mGridXY + arrayPos.y * mGridSize.x + arrayPos.x] * (1.0f - fracPart.x) +
                mGrid[(arrayPos.z + 1) * mGridXY + arrayPos.y * mGridSize.x + arrayPos.x + 1] * fracPart.x;
    float d11 = mGrid[(arrayPos.z + 1) * mGridXY + (arrayPos.y + 1) * mGridSize.x + arrayPos.x] * (1.0f - fracPart.x) +
                mGrid[(arrayPos.z + 1) * mGridXY + (arrayPos.y + 1) * mGridSize.x + arrayPos.x + 1] * fracPart.x;

    float d0 = d00 * (1.0f - fracPart.y) + d01 * fracPart.y;
    float d1 = d10 * (1.0f - fracPart.y) + d11 * fracPart.y;

    return d0 * (1.0f - fracPart.z) + d1 * fracPart.z;
}