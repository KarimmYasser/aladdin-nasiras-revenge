#include "mesh-utils.hpp"

#include <glm/gtc/constants.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void pushCornerVertex(const tinyobj::attrib_t& attrib,
                      const tinyobj::index_t& idx,
                      std::unordered_map<our::Vertex, GLuint>& vertex_map,
                      std::vector<our::Vertex>& vertices,
                      std::vector<GLuint>& elements) {
    our::Vertex vertex = {};

    vertex.position = {
        attrib.vertices[3 * size_t(idx.vertex_index) + 0],
        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
        attrib.vertices[3 * size_t(idx.vertex_index) + 2],
    };

    if (idx.normal_index >= 0 &&
        static_cast<size_t>(3 * idx.normal_index + 2) < attrib.normals.size()) {
        vertex.normal = {
            attrib.normals[3 * size_t(idx.normal_index) + 0],
            attrib.normals[3 * size_t(idx.normal_index) + 1],
            attrib.normals[3 * size_t(idx.normal_index) + 2],
        };
    } else {
        vertex.normal = {0.0f, 1.0f, 0.0f};
    }

    if (idx.texcoord_index >= 0 &&
        static_cast<size_t>(2 * idx.texcoord_index + 1) < attrib.texcoords.size()) {
        vertex.tex_coord = {
            attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
            attrib.texcoords[2 * size_t(idx.texcoord_index) + 1],
        };
    } else {
        vertex.tex_coord = {0.0f, 0.0f};
    }

    const size_t colorBase = static_cast<size_t>(3 * idx.vertex_index);
    if (colorBase + 2 < attrib.colors.size()) {
        vertex.color = {
            static_cast<glm::uint8>(attrib.colors[colorBase + 0] * 255.0f),
            static_cast<glm::uint8>(attrib.colors[colorBase + 1] * 255.0f),
            static_cast<glm::uint8>(attrib.colors[colorBase + 2] * 255.0f),
            255,
        };
    } else {
        vertex.color = {255, 255, 255, 255};
    }

    auto it = vertex_map.find(vertex);
    if (it == vertex_map.end()) {
        auto new_vertex_index = static_cast<GLuint>(vertices.size());
        vertex_map[vertex] = new_vertex_index;
        elements.push_back(new_vertex_index);
        vertices.push_back(vertex);
    } else {
        elements.push_back(it->second);
    }
}

} // namespace

our::Mesh* our::mesh_utils::loadOBJ(const std::string& filename) {

    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;
    std::unordered_map<our::Vertex, GLuint> vertex_map;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string mtlBaseDir;
    const size_t pathSep = filename.find_last_of("/\\");
    if (pathSep != std::string::npos) {
        mtlBaseDir = filename.substr(0, pathSep + 1);
    }
    const char* mtlDir = mtlBaseDir.empty() ? nullptr : mtlBaseDir.c_str();

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), mtlDir)) {
        std::cerr << "Failed to load obj file \"" << filename << "\" due to error: " << err << std::endl;
        return nullptr;
    }
    if (!warn.empty()) {
        std::cout << "WARN while loading obj file \"" << filename << "\": " << warn << std::endl;
    }

    std::vector<MeshSubmesh> submeshes;
    std::string prevMtl;
    size_t rangeStart = 0;

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            const int fv = shape.mesh.num_face_vertices[f];
            int matId = -1;
            if (f < shape.mesh.material_ids.size()) {
                matId = shape.mesh.material_ids[f];
            }
            std::string mtlName = "default";
            if (matId >= 0 && matId < static_cast<int>(materials.size())) {
                mtlName = materials[size_t(matId)].name;
            }

            if (fv < 3) {
                index_offset += size_t(fv);
                continue;
            }

            for (int tri = 0; tri < fv - 2; tri++) {
                if (mtlName != prevMtl) {
                    if (elements.size() > rangeStart) {
                        MeshSubmesh sm{};
                        sm.firstIndex = static_cast<GLuint>(rangeStart);
                        sm.indexCount = static_cast<GLsizei>(elements.size() - rangeStart);
                        sm.materialName = prevMtl.empty() ? std::string("default") : prevMtl;
                        submeshes.push_back(sm);
                    }
                    rangeStart = elements.size();
                    prevMtl = mtlName;
                }

                const tinyobj::index_t& i0 = shape.mesh.indices[index_offset + 0];
                const tinyobj::index_t& i1 = shape.mesh.indices[index_offset + size_t(tri + 1)];
                const tinyobj::index_t& i2 = shape.mesh.indices[index_offset + size_t(tri + 2)];
                pushCornerVertex(attrib, i0, vertex_map, vertices, elements);
                pushCornerVertex(attrib, i1, vertex_map, vertices, elements);
                pushCornerVertex(attrib, i2, vertex_map, vertices, elements);
            }

            index_offset += size_t(fv);
        }
    }

    if (elements.size() > rangeStart) {
        MeshSubmesh sm{};
        sm.firstIndex = static_cast<GLuint>(rangeStart);
        sm.indexCount = static_cast<GLsizei>(elements.size() - rangeStart);
        sm.materialName = prevMtl.empty() ? std::string("default") : prevMtl;
        submeshes.push_back(sm);
    }

    if (elements.empty()) {
        std::cerr << "OBJ has no triangle data: \"" << filename << "\"" << std::endl;
        return nullptr;
    }

    std::vector<float> physicsPos;
    physicsPos.reserve(vertices.size() * 3);
    for (const auto& v : vertices) {
        physicsPos.push_back(v.position.x);
        physicsPos.push_back(v.position.y);
        physicsPos.push_back(v.position.z);
    }
    std::vector<uint32_t> physicsIdx(elements.begin(), elements.end());

    return new our::Mesh(vertices, elements, std::move(submeshes), std::move(physicsPos), std::move(physicsIdx));
}

our::Mesh* our::mesh_utils::sphere(const glm::ivec2& segments) {
    std::vector<our::Vertex> vertices;
    std::vector<unsigned int> elements;

    for (int lat = 0; lat <= segments.y; lat++) {
        float v = (float)lat / segments.y;
        float pitch = v * glm::pi<float>() - glm::half_pi<float>();
        float cos = glm::cos(pitch), sin = glm::sin(pitch);
        for (int lng = 0; lng <= segments.x; lng++) {
            float u = (float)lng / segments.x;
            float yaw = u * glm::two_pi<float>();
            glm::vec3 normal = {cos * glm::cos(yaw), sin, cos * glm::sin(yaw)};
            glm::vec3 position = normal;
            glm::vec2 tex_coords = glm::vec2(u, v);
            our::Color color = our::Color(255, 255, 255, 255);
            vertices.push_back({position, color, tex_coords, normal});
        }
    }

    for (int lat = 1; lat <= segments.y; lat++) {
        int start = lat * (segments.x + 1);
        for (int lng = 1; lng <= segments.x; lng++) {
            int prev_lng = lng - 1;
            elements.push_back(lng + start);
            elements.push_back(lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start);
            elements.push_back(lng + start);
        }
    }

    return new our::Mesh(vertices, elements);
}
