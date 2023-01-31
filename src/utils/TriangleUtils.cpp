#include "TriangleUtils.h"

namespace TriangleUtils
{
    std::vector<TriangleData> calculateMeshTriangleData(const Mesh& mesh)
    {
        const std::vector<glm::vec3>& vertices = mesh.getVertices();
        const std::vector<uint32_t> indices = mesh.getIndices();

        std::vector<TriangleData> triangles(indices.size()/3);
        std::vector<bool> isTriangleDegenerated(indices.size()/3, false);
        std::vector<std::pair<uint32_t, uint32_t>> degeneratedTriangles; // Stores triangle index and vertex index with bigger angle

        // Cache structures
        std::map<std::pair<uint32_t, uint32_t>, uint32_t> edgesNormal;
        std::vector<glm::vec3> verticesNormal(vertices.size(), glm::vec3(0.0f));

        // Init triangles
        uint32_t numDegeneratedTriangles = 0;
		for (int i = 0, tIndex = 0; i < indices.size(); i += 3, tIndex++)
		{
            // Mark area zero triangles
            const double zeroAngleThreshold = 1e-5;
            const double degeneratedTriangleValue = 0.02;
            double triangleArea = 0.5f * glm::length(glm::cross(static_cast<glm::dvec3>(vertices[indices[i + 1]] - vertices[indices[i]]), static_cast<glm::dvec3>(vertices[indices[i + 2]] - vertices[indices[i]])));
            double maxTriangleBase = 0.0f;
            uint32_t degeneratedVertex = 0;
            for(int k=0; k < 3; k++)
            {
                const uint32_t v1 = indices[i + k];
                const uint32_t v2 = indices[i + ((k+1) % 3)];

                double triangleBase = glm::length(vertices[v2] - vertices[v1]);
                if(triangleBase > maxTriangleBase)
                {
                    maxTriangleBase = triangleBase;
                    degeneratedVertex = k;
                }
            }

            const double maxTriangleHeighValue = glm::tan(glm::radians(30.0));
            double triangleDegerancyValue = 2.0f * triangleArea * maxTriangleHeighValue / (maxTriangleBase * maxTriangleBase);

            if(triangleArea < zeroAngleThreshold && triangleDegerancyValue < degeneratedTriangleValue)
            {
                degeneratedTriangles.push_back(std::make_pair(tIndex, degeneratedVertex));
                isTriangleDegenerated[tIndex] = true;
                numDegeneratedTriangles++;
                triangles[tIndex] = TriangleUtils::TriangleData(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
                triangles[tIndex].transform[0][2] = 0.0f;
                triangles[tIndex].transform[1][2] = 0.0f;
                triangles[tIndex].transform[2][2] = 0.0f;
            }
            else
            {
                triangles[tIndex] = TriangleUtils::TriangleData(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
            }

            if(glm::isnan(triangles[tIndex].getTriangleNormal().x) ||
               glm::isnan(triangles[tIndex].getTriangleNormal().y) ||
               glm::isnan(triangles[tIndex].getTriangleNormal().z))
            {
                std::cout << "is nan11" << std::endl;
            }
		}

        if(numDegeneratedTriangles > 0)
        {
            SPDLOG_INFO("The mesh has {} degenerated triangles", numDegeneratedTriangles);
        }

        for(int i = 0, tIndex = 0; i < indices.size(); i += 3, tIndex++)
        {
            if(isTriangleDegenerated[tIndex]) continue;
            for(int k=0; k < 3; k++)
            {
                const uint32_t v1 = indices[i + k];
                const uint32_t v2 = indices[i + ((k+1) % 3)];
                const uint32_t v3 = indices[i + ((k+2) % 3)];
                std::pair<std::map<std::pair<uint32_t, uint32_t>, uint32_t>::iterator, bool> ret;
                ret = edgesNormal.insert(std::make_pair(
                                            std::make_pair(glm::min(v1, v2), glm::max(v1, v2)), i + k));
                if(!ret.second)
                {
					const uint32_t t2Index = ret.first->second / 3;

                    glm::vec3 edgeNormal = triangles[tIndex].getTriangleNormal() + triangles[t2Index].getTriangleNormal();

                    triangles[tIndex].edgesNormal[k] = triangles[tIndex].transform * edgeNormal;
                    triangles[t2Index].edgesNormal[ret.first->second % 3] = triangles[t2Index].transform * edgeNormal;
                    edgesNormal.erase(ret.first);
                }

                const float angle = glm::acos(glm::clamp(glm::dot(glm::normalize(vertices[v2] - vertices[v1]), glm::normalize(vertices[v3] - vertices[v1])), -1.0f, 1.0f));
                verticesNormal[v1] += angle * triangles[tIndex].getTriangleNormal();
            }
        }

        // Interpret degenerated triangles as edges
        std::vector<glm::vec3> degeneratedTrianglesNormal(degeneratedTriangles.size());
        for(std::pair<uint32_t, uint32_t> degTri : degeneratedTriangles)
        {
            const uint32_t tIndex = degTri.first;

            const uint32_t v1 = indices[3 * tIndex + degTri.second];
            const uint32_t v2 = indices[3 * tIndex + ((degTri.second + 1) % 3)];
            const uint32_t v3 = indices[3 * tIndex + ((degTri.second + 2) % 3)];
            
            glm::vec3 edgeNormal(0.0f);
            std::array<uint32_t, 3> adjEdgeFaceIndices;
            adjEdgeFaceIndices.fill(std::numeric_limits<uint32_t>::max());

            bool validNeighbours = true;
            auto e1it = edgesNormal.find(std::make_pair(glm::min(v1, v2), glm::max(v1, v2)));
            if(e1it != edgesNormal.end())
            {
                const uint32_t t2Index = e1it->second / 3;
                validNeighbours = validNeighbours && !isTriangleDegenerated[t2Index];
                edgeNormal += triangles[t2Index].getTriangleNormal();
                adjEdgeFaceIndices[0] = e1it->second;
            }
            else validNeighbours = false;

            auto e2it = edgesNormal.find(std::make_pair(glm::min(v2, v3), glm::max(v2, v3)));
            auto e3it = edgesNormal.find(std::make_pair(glm::min(v3, v1), glm::max(v3, v1)));
            auto longestEdgeIt = (glm::length(v3-v2) > glm::length(v1-v3)) ? e2it : e3it;
            if(longestEdgeIt != edgesNormal.end())
            {
                const uint32_t t2Index = longestEdgeIt->second / 3;
                validNeighbours = validNeighbours && !isTriangleDegenerated[t2Index];
                edgeNormal += triangles[t2Index].getTriangleNormal();
            }
            else validNeighbours = false;

            if(validNeighbours)
            {
                edgesNormal.erase(e1it);
                if(e2it != edgesNormal.end())
                {
                    adjEdgeFaceIndices[1] = e2it->second;
                    edgesNormal.erase(e2it);
                } 
                    
                if(e3it != edgesNormal.end())
                {
                    adjEdgeFaceIndices[2] = e3it->second;
                    edgesNormal.erase(e3it);
                }

                for(uint32_t k=0; k < 3; k++)
                {
                    const uint32_t idx = adjEdgeFaceIndices[k];
                    const uint32_t t2Index = idx/3;
                    if(t2Index >= triangles.size()) continue;
                    if(!isTriangleDegenerated[t2Index])
                        triangles[t2Index].edgesNormal[idx % 3] = triangles[t2Index].transform * edgeNormal;
                }

                for(uint32_t k=0; k < 3; k++)
                {
                    const uint32_t v1 = indices[3 * tIndex + k];
                    const uint32_t v2 = indices[3 * tIndex + ((k+1) % 3)];
                    const uint32_t v3 = indices[3 * tIndex + ((k+2) % 3)];

                    const float angle = glm::acos(glm::clamp(glm::dot(glm::normalize(vertices[v2] - vertices[v1]), glm::normalize(vertices[v3] - vertices[v1])), -1.0f, 1.0f));
                    verticesNormal[v1] += angle * edgeNormal;
                }
            }
            else
            {
                
            }
        }

        if(edgesNormal.size() > 0)
        {
            SPDLOG_INFO("The mesh has {} non-maifold edges, trying to merge near vertices", edgesNormal.size());
            std::map<uint32_t, uint32_t> verticesMap;
            auto findVertexParent = [&] (uint32_t vId) -> uint32_t
            {
                auto it = verticesMap.find(vId);
                while(it != verticesMap.end() && it->second != vId)
                {
                    vId = it->second;
                    it = verticesMap.find(vId);
                }
                return vId;
            };

            std::vector<uint32_t> nonManifoldVertices(2 * edgesNormal.size());
            uint32_t index = 0;
            for(auto& elem : edgesNormal)
            {
                nonManifoldVertices[index++] = elem.first.first;
                nonManifoldVertices[index++] = elem.first.second;
            }

            std::sort(nonManifoldVertices.begin(), nonManifoldVertices.end());
            auto newEnd = std::unique(nonManifoldVertices.begin(), nonManifoldVertices.end());
            nonManifoldVertices.erase(newEnd, nonManifoldVertices.end());

            // Generate a possible vertex mapping
            for(uint32_t i=0; i < nonManifoldVertices.size(); i++)
            {
                const glm::vec3& v1 = vertices[nonManifoldVertices[i]];
                for(uint32_t ii=i+1; ii < nonManifoldVertices.size(); ii++)
                {
                    const glm::vec3& diff = v1 - vertices[nonManifoldVertices[ii]];
                    if(glm::dot(diff, diff) < 0.00001f)
                    {
                        uint32_t p1 = findVertexParent(nonManifoldVertices[i]);
                        uint32_t p2 = findVertexParent(nonManifoldVertices[ii]);

                        if(nonManifoldVertices[i] == p1) verticesMap[p1] = p1;
                        verticesMap[p2] = p1;
                        break;
                    }
                }
            }

            std::map<std::pair<uint32_t, uint32_t>, uint32_t> newEdgesNormals;
            auto it = edgesNormal.begin();
            for(; it != edgesNormal.end(); it++)
            {
                const uint32_t v1 = findVertexParent(it->first.first);
                const uint32_t v2 = findVertexParent(it->first.second);

                std::pair<std::map<std::pair<uint32_t, uint32_t>, uint32_t>::iterator, bool> ret;
                ret = newEdgesNormals.insert(std::make_pair(
                                            std::make_pair(glm::min(v1, v2), glm::max(v1, v2)), it->second));
                if(!ret.second)
                {
                    const uint32_t tIndex = it->second / 3;
					const uint32_t t2Index = ret.first->second / 3;
                    glm::vec3 edgeNormal = triangles[tIndex].getTriangleNormal() + triangles[t2Index].getTriangleNormal();
                    triangles[tIndex].edgesNormal[it->second % 3] = triangles[tIndex].transform * edgeNormal;
                    triangles[t2Index].edgesNormal[ret.first->second % 3] = triangles[t2Index].transform * edgeNormal;
                    newEdgesNormals.erase(ret.first);
                }
            }

            // Calculate parent real normals
            for(uint32_t vId : nonManifoldVertices)
            {
                uint32_t p = findVertexParent(vId);
                if(p != vId) verticesNormal[p] += verticesNormal[vId];
            }

            // Propagate parent normals
            for(uint32_t vId : nonManifoldVertices)
            {
                uint32_t p = findVertexParent(vId);
                verticesNormal[vId] = verticesNormal[p];
            }

            if(newEdgesNormals.size() > 0)
            {
                SPDLOG_ERROR("The mesh has {} non-maifold edges that cannot be merged", edgesNormal.size());
            }
            else
            {
                SPDLOG_INFO("All the non-manifold vertices merged correctly");
            }
        }

        for(int i = 0; i < indices.size(); i++)
        {
            triangles[i/3].verticesNormal[i % 3] = triangles[i/3].transform * verticesNormal[indices[i]];
        }

        return triangles;
    }
}