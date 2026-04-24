#include "skinned-mesh.hpp"

namespace our {

    // -----------------------------------------------------------------
    // Constructor – uploads vertex and index data to VRAM and configures
    // the VAO with all 6 attribute pointers.
    // -----------------------------------------------------------------
    SkinnedMesh::SkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                             const std::vector<unsigned int>&  elements,
                             const std::vector<Submesh>&       submeshes)
        : submeshes(submeshes)
    {
        elementCount = static_cast<GLsizei>(elements.size());

        // Create and bind the Vertex Array Object first so every buffer
        // and attribute call below is recorded into it.
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // ---- Vertex buffer ----
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(SkinnedVertex)),
                     vertices.data(),
                     GL_STATIC_DRAW);   // geometry is static; bone matrices go via uniforms

        // ---- Index buffer ----
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(elements.size() * sizeof(unsigned int)),
                     elements.data(),
                     GL_STATIC_DRAW);

        // ---- Attribute 0 : position (vec3) ----
        glEnableVertexAttribArray(ATTRIB_LOC_POSITION);
        glVertexAttribPointer(ATTRIB_LOC_POSITION,
                              3, GL_FLOAT, GL_FALSE,
                              sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, position));

        // ---- Attribute 1 : color (vec4 float) ----
        glEnableVertexAttribArray(ATTRIB_LOC_COLOR);
        glVertexAttribPointer(ATTRIB_LOC_COLOR,
                              4, GL_FLOAT, GL_FALSE,
                              sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, color));

        // ---- Attribute 2 : tex_coord (vec2) ----
        glEnableVertexAttribArray(ATTRIB_LOC_TEXCOORD);
        glVertexAttribPointer(ATTRIB_LOC_TEXCOORD,
                              2, GL_FLOAT, GL_FALSE,
                              sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, tex_coord));

        // ---- Attribute 3 : normal (vec3) ----
        glEnableVertexAttribArray(ATTRIB_LOC_NORMAL);
        glVertexAttribPointer(ATTRIB_LOC_NORMAL,
                              3, GL_FLOAT, GL_FALSE,
                              sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, normal));

        // ---- Attribute 4 : boneIDs (ivec4) ----
        // IMPORTANT: must use glVertexAttribIPointer (the "I" variant) so that
        // integer data is passed as integers to the shader (ivec4), not converted
        // to floats.  Using the regular glVertexAttribPointer here would corrupt
        // the bone index values.
        glEnableVertexAttribArray(ATTRIB_LOC_BONE_IDS);
        glVertexAttribIPointer(ATTRIB_LOC_BONE_IDS,
                               4, GL_INT,
                               sizeof(SkinnedVertex),
                               (void*)offsetof(SkinnedVertex, boneIDs));

        // ---- Attribute 5 : boneWeights (vec4) ----
        glEnableVertexAttribArray(ATTRIB_LOC_BONE_WEIGHTS);
        glVertexAttribPointer(ATTRIB_LOC_BONE_WEIGHTS,
                              4, GL_FLOAT, GL_FALSE,
                              sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, boneWeights));

        // Unbind VAO to prevent accidental modification from outside.
        glBindVertexArray(0);
    }

    // -----------------------------------------------------------------
    // draw – bind the VAO and issue one indexed draw call.
    // The caller is responsible for binding the shader and uploading
    // the finalBoneMatrices[] uniform array before calling this.
    // -----------------------------------------------------------------
    void SkinnedMesh::draw()
    {
        glBindVertexArray(VAO);
        if (submeshes.empty()) {
            glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, nullptr);
        } else {
            for (const auto& sub : submeshes) {
                glDrawElements(GL_TRIANGLES, sub.elementCount, GL_UNSIGNED_INT, sub.elementOffset);
            }
        }
        glBindVertexArray(0);
    }

    void SkinnedMesh::drawSubmesh(int index)
    {
        if (index < 0 || index >= (int)submeshes.size()) return;
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, submeshes[index].elementCount, GL_UNSIGNED_INT, submeshes[index].elementOffset);
        glBindVertexArray(0);
    }

    // -----------------------------------------------------------------
    // Destructor – release GPU resources.
    // -----------------------------------------------------------------
    SkinnedMesh::~SkinnedMesh()
    {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }

} // namespace our
