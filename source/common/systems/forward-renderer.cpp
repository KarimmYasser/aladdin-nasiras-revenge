#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../material/lit-material.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <iostream>


#define MAX_LIGHTS 8

namespace our {

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config){
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if(config.contains("sky")){
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));
            
            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            //TODO: (Req 10) Pick the correct pipeline state to draw the sky
            // Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            // We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky 
            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material 
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 0.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if(config.contains("postprocess")){
            //TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            //TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            // Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            // The depth format can be (Depth component with 24 bits).
            colorTarget = texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if(framebufferStatus != GL_FRAMEBUFFER_COMPLETE){
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                throw std::runtime_error("Postprocess framebuffer is incomplete.");
            }

            //TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }

        // -----------------------------------------------------------------------
        // Shadow map resources
        // A 2048×2048 depth-only texture is used as the shadow map for the primary
        // directional light. Fragments outside the light frustum are clamped to the
        // border value (1.0 = max depth = not in shadow).
        // -----------------------------------------------------------------------
        glGenTextures(1, &shadowDepthTexture);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Clamp to border = 1.0 so fragments outside the shadow frustum are treated as lit.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float shadowBorderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, shadowBorderColor);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &shadowFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowDepthTexture, 0);
        // No colour attachment — depth only.
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        shadowShader = new ShaderProgram();
        shadowShader->attach("assets/shaders/shadow.vert", GL_VERTEX_SHADER);
        shadowShader->attach("assets/shaders/shadow.frag", GL_FRAGMENT_SHADER);
        shadowShader->link();

        // Skinned mesh shader: same fragment shader as lit objects, custom vertex shader.
        skinnedShader = new ShaderProgram();
        skinnedShader->attach("assets/shaders/skinned.vert", GL_VERTEX_SHADER);
        skinnedShader->attach("assets/shaders/light.frag",   GL_FRAGMENT_SHADER);
        skinnedShader->link();
    }

    void ForwardRenderer::destroy(){
        // Delete all objects related to the sky
        if(skyMaterial){
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if(postprocessMaterial){
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
        // Delete shadow mapping resources
        if(shadowFBO)          { glDeleteFramebuffers(1, &shadowFBO);  shadowFBO = 0; }
        if(shadowDepthTexture) { glDeleteTextures(1, &shadowDepthTexture); shadowDepthTexture = 0; }
        if(shadowShader)       { delete shadowShader; shadowShader = nullptr; }
        if(skinnedShader)      { delete skinnedShader; skinnedShader = nullptr; }
    }

    void ForwardRenderer::render(World* world){
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent* camera = nullptr;
        // Clear light list every frame before collecting fresh ones
        lights.clear();
        opaqueCommands.clear();
        transparentCommands.clear();
        for(auto entity : world->getEntities()){
            // If we hadn't found a camera yet, we look for a camera in this entity
            if(!camera) camera = entity->getComponent<CameraComponent>();
            // Collect any light attached to this entity
            if(auto lightComp = entity->getComponent<LightComponent>(); lightComp){
                lights.push_back(lightComp);
            }
            // If this entity has a mesh renderer component
            if(auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer && meshRenderer->visible){
                // Skip entities handled by SkinnedMeshRendererComponent (they have their own dedicated pass)
                if (entity->getComponent<SkinnedMeshRendererComponent>()) continue;
                // Also skip entities that still use the legacy AnimatorComponent
                if (entity->getComponent<AnimatorComponent>()) continue;

                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;

                // if it is transparent, we add it to the transparent commands list
                if(command.material->transparent){
                    transparentCommands.push_back(command);
                } else {
                // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if(camera == nullptr) return;

        //TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        // The camera's local forward is (0,0,-1). Transform it to world space using the LocalToWorld matrix.
        auto camM = camera->getOwner()->getLocalToWorldMatrix();
        glm::vec3 cameraForward = glm::vec3(camM * glm::vec4(0, 0, -1, 0));
        glm::vec3 cameraPos = glm::vec3(camM[3]);

        //TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        // Find the first directional light and render all opaque geometry from its perspective into the shadow depth map.
        LightComponent* shadowCaster = nullptr;
        for (auto* lc : lights) {
            if (lc->type == LightComponent::LightType::DIRECTIONAL) {
                shadowCaster = lc;
                break;
            }
        }

        shadowEnabled = (shadowCaster != nullptr && !opaqueCommands.empty());
        if (shadowEnabled) {
            glm::mat4 lM  = shadowCaster->getOwner()->getLocalToWorldMatrix();
            glm::vec3 lDir = glm::normalize(glm::vec3(lM * glm::vec4(0, 0, -1, 0)));

            glm::vec3 up = (lDir.y > 0.99f || lDir.y < -0.99f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
            
            // Center the shadow frustum on the camera position so shadows follow the player as they move
            glm::vec3 lEye = cameraPos - lDir * 20.0f;
            glm::mat4 lightView = glm::lookAt(lEye, lEye + lDir, up);

            // Orthographic projection covers ±20 units around the camera.
            float range = 20.0f;
            glm::mat4 lightProj = glm::ortho(-range, range, -range, range, 1.0f, 50.0f);
            lightSpaceMatrix = lightProj * lightView;

            // Render the scene depth from the light's point of view.
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
            
            // CRITICAL FIX: Explicitly enable depth writing. 
            // If a previous frame's post-processing disabled glDepthMask, clear and draw will fail.
            glDepthMask(GL_TRUE); 
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            shadowShader->use();
            for (const auto& command : opaqueCommands) {
                shadowShader->set("light_space_matrix", lightSpaceMatrix);
                shadowShader->set("model", command.localToWorld);
                command.mesh->draw();
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand& first, const RenderCommand& second){
            //TODO: (Req 9) Finish this function
            // We sort transparent objects from FAR to NEAR (back-to-front / painter's algorithm)
            // "distance along cameraForward" = dot(center, cameraForward)
            // A larger dot product means the object is further in the forward direction => draw it first
            return glm::dot(first.center, cameraForward) > glm::dot(second.center, cameraForward);
        });

        // upload all collected lights to the shader currently bound by a LitMaterial.
        ShaderProgram* lastLitShader = nullptr; // Optimization: track last shader to avoid re-uploading lights
        auto uploadLights = [&](ShaderProgram* shader) {
            if (shader == lastLitShader) return; // already uploaded for this shader this frame
            lastLitShader = shader;

            shader->set("eye_pos",    cameraPos);
            shader->set("light_count", (GLint)std::min((int)lights.size(), MAX_LIGHTS));
            for (int i = 0; i < (int)lights.size() && i < MAX_LIGHTS; ++i) {
                LightComponent* lc = lights[i];
                glm::mat4 lM = lc->getOwner()->getLocalToWorldMatrix();
                glm::vec3 lPos = glm::vec3(lM[3]);
                glm::vec3 lDir = glm::normalize(glm::vec3(lM * glm::vec4(0, 0, -1, 0)));

                std::string base = "lights[" + std::to_string(i) + "]";
                shader->set(base + ".type",       static_cast<GLint>(lc->type));
                shader->set(base + ".position",   lPos);
                shader->set(base + ".direction",  lDir);
                shader->set(base + ".color",      lc->color);
                shader->set(base + ".intensity",  lc->intensity);
                shader->set(base + ".att_constant",  lc->attenuation_constant);
                shader->set(base + ".att_linear",    lc->attenuation_linear);
                shader->set(base + ".att_quadratic", lc->attenuation_quadratic);
                // Convert cone angles from degrees to cosines for the GLSL smoothstep comparison
                shader->set(base + ".inner_cutoff", std::cos(glm::radians(lc->inner_angle)));
                shader->set(base + ".outer_cutoff", std::cos(glm::radians(lc->outer_angle)));
            }

            // Shadow uniforms — unit 3
            shader->set("shadow_enabled",    (GLint)shadowEnabled);
            shader->set("shadow_map",        (GLint)3);
            shader->set("light_space_matrix", lightSpaceMatrix);
        };

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
        glActiveTexture(GL_TEXTURE0); // restore default active unit
        
        //TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        glViewport(0, 0, windowSize.x, windowSize.y);
        
        //TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);
        
        //TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(true, true, true, true); // allow writing to all color channels
        glDepthMask(true);                   // allow writing to depth buffer

        // If there is a postprocess material, bind the framebuffer
        if(postprocessMaterial){
            //TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        //TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //TODO: (Req 9) Draw all the opaque commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for(const auto& command : opaqueCommands){
            command.material->setup();
            command.material->shader->set("transform", VP * command.localToWorld);
            // If the material is a LitMaterial, also upload the model matrix, normal matrix, and lights
            if (auto* litMat = dynamic_cast<LitMaterial*>(command.material)) {
                ShaderProgram* sh = command.material->shader;
                uploadLights(sh);
                sh->set("model", command.localToWorld);
                glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(command.localToWorld)));
                glUniformMatrix3fv(sh->getUniformLocation("normal_mat"), 1, GL_FALSE, &normalMat[0][0]);
                
                // If this command has bones, upload them to the shader
                if(!command.bones.empty()){
                    for (int i = 0; i < (int)command.bones.size() && i < 100; i++) {
                        sh->set("finalBoneMatrices[" + std::to_string(i) + "]", command.bones[i]);
                    }
                }
            }
            command.mesh->draw();
        }
        // If there is a sky material, draw the sky
        if(this->skyMaterial){
            //TODO: (Req 10) setup the sky material
            this->skyMaterial->setup();

            //TODO: (Req 10) Create a model matrix for the sy such that it always follows the camera (sky sphere center = camera position)
            glm::mat4 skyModel = glm::translate(glm::mat4(1.0f), cameraPos);

            //TODO: (Req 10) We want the sky to be drawn behind everything (in NDC space, z=1)
            // We can acheive the is by multiplying by an extra matrix after the projection but what values should we put in it?
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f, // col 0
                0.0f, 1.0f, 0.0f, 0.0f, // col 1
                0.0f, 0.0f, 0.0f, 0.0f, // col 2 (z becomes 0)
                0.0f, 0.0f, 1.0f, 1.0f  // col 3 (w goes into z and w)
            );
            //TODO: (Req 10) set the "transform" uniform
            this->skyMaterial->shader->set("transform", alwaysBehindTransform * VP * skyModel);

            //TODO: (Req 10) draw the sky sphere
            this->skySphere->draw();
        }
        //TODO: (Req 9) Draw all the transparent commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for(const auto& command : transparentCommands){
            command.material->setup();
            command.material->shader->set("transform", VP * command.localToWorld);
            if (auto* litMat = dynamic_cast<LitMaterial*>(command.material)) {
                ShaderProgram* sh = command.material->shader;
                uploadLights(sh);
                sh->set("model", command.localToWorld);
                glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(command.localToWorld)));
                glUniformMatrix3fv(sh->getUniformLocation("normal_mat"), 1, GL_FALSE, &normalMat[0][0]);
                
                // If this command has bones, upload them to the shader
                if(!command.bones.empty()){
                    for (int i = 0; i < (int)command.bones.size() && i < 100; i++) {
                        sh->set("finalBoneMatrices[" + std::to_string(i) + "]", command.bones[i]);
                    }
                }
            }
            command.mesh->draw();
        }

        // ── Skinned mesh pass ───────────────────────────────────────────────────
        // Draws entities that have a SkinnedMeshRendererComponent using the
        // GPU skinning vertex shader.  Legacy AnimatorComponent entities are
        // also handled here for backward compatibility.
        if (skinnedShader) {
            skinnedShader->use();
            uploadLights(skinnedShader);  // eye_pos, lights[], shadow uniforms

            // Rebind shadow map on unit 3 for the skinned pass
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
            glActiveTexture(GL_TEXTURE0);

            for (auto entity : world->getEntities()) {

                // ── New merged component path ─────────────────────────────────
                if (auto* smr = entity->getComponent<SkinnedMeshRendererComponent>()) {
                    if (!smr->skinnedMesh || !smr->visible) continue;

                    glm::mat4 model = entity->getLocalToWorldMatrix();
                    glm::mat3 nrm   = glm::mat3(glm::transpose(glm::inverse(model)));

                    if (entity->name == "aladdin") {
                        static int frameCount = 0;
                        if (frameCount++ % 100 == 0) {
                            std::cout << "[DIAG][ForwardRenderer] Rendering aladdin: pos=" << model[3][0] << "," << model[3][1] << "," << model[3][2] 
                                      << " scale=" << glm::length(glm::vec3(model[0])) << " visible=" << smr->visible 
                                      << " bones=" << smr->animator.getFinalBoneMatrices().size() << std::endl;
                        }
                    }

                    skinnedShader->set("transform",          VP * model);
                    skinnedShader->set("model",              model);
                    glUniformMatrix3fv(skinnedShader->getUniformLocation("normal_mat"),
                                       1, GL_FALSE, &nrm[0][0]);
                    skinnedShader->set("light_space_matrix", lightSpaceMatrix);

                    const auto& mats = smr->animator.getFinalBoneMatrices();
                    if (!mats.empty()) {
                        GLint loc = skinnedShader->getUniformLocation("finalBoneMatrices[0]");
                        if (loc != -1) {
                            glUniformMatrix4fv(loc, (GLsizei)std::min((size_t)mats.size(), (size_t)64), GL_FALSE, &mats[0][0][0]);
                        }
                    }

                    // Bind material textures & set uniforms ON THE SKINNED SHADER for each submesh.
                    for (int i = 0; i < (int)smr->materials.size(); i++) {
                        Material* mat = smr->materials[i];
                        if (!mat) continue;

                        if (auto* litMat = dynamic_cast<LitMaterial*>(mat)) {
                            litMat->pipelineState.setup();
                            skinnedShader->use(); // ensure we're still on the skinned program

                            // Re-bind shadow map on unit 3 (pipelineState.setup() may reset state)
                            glActiveTexture(GL_TEXTURE3);
                            glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

                            // Albedo (unit 0)
                            static auto* white = our::texture_utils::singleColor({255, 255, 255, 255});
                            static auto* black = our::texture_utils::singleColor({0, 0, 0, 255});
                            glActiveTexture(GL_TEXTURE0);
                            (litMat->albedo_map ? litMat->albedo_map : white)->bind();
                            if (litMat->sampler) litMat->sampler->bind(0);
                            skinnedShader->set("material.albedo_map", (GLint)0);

                            // Specular (unit 1)
                            glActiveTexture(GL_TEXTURE1);
                            (litMat->specular_map ? litMat->specular_map : black)->bind();
                            if (litMat->sampler) litMat->sampler->bind(1);
                            skinnedShader->set("material.specular_map", (GLint)1);

                            // Emission (unit 2)
                            glActiveTexture(GL_TEXTURE2);
                            (litMat->emission_map ? litMat->emission_map : black)->bind();
                            if (litMat->sampler) litMat->sampler->bind(2);
                            skinnedShader->set("material.emission_map", (GLint)2);

                            // Shadow (unit 3) uniform
                            skinnedShader->set("shadow_map", (GLint)3);

                            skinnedShader->set("material.shininess", litMat->shininess);
                            skinnedShader->set("material.ambient", litMat->ambient);
                            skinnedShader->set("material.albedo_tint", litMat->albedo_tint);
                            skinnedShader->set("uv_multiplier", litMat->uv_multiplier);

                            glActiveTexture(GL_TEXTURE0);
                        } else {
                            // Non-lit material fallback: let it set up, then rebind our shader
                            mat->setup();
                            skinnedShader->use();
                        }

                        if (i < (int)smr->skinnedMesh->submeshes.size()) {
                            smr->skinnedMesh->drawSubmesh(i);
                        } else if (i == 0) {
                            // Fallback if no submeshes defined (unlikely with my loader update)
                            smr->skinnedMesh->draw();
                        }
                    }
                    continue; // handled — skip legacy path below
                }

                // ── Legacy AnimatorComponent path (backward compat) ───────────
                auto* anim = entity->getComponent<AnimatorComponent>();
                if (!anim || !anim->skinnedMesh) continue;

                glm::mat4 model  = entity->getLocalToWorldMatrix();
                glm::mat3 nrm    = glm::mat3(glm::transpose(glm::inverse(model)));

                skinnedShader->set("transform",          VP * model);
                skinnedShader->set("model",              model);
                glUniformMatrix3fv(skinnedShader->getUniformLocation("normal_mat"),
                                   1, GL_FALSE, &nrm[0][0]);
                skinnedShader->set("light_space_matrix", lightSpaceMatrix);

                const auto& mats = anim->animator.getFinalBoneMatrices();
                for (int i = 0; i < (int)mats.size() && i < Animator::MAX_BONES; i++)
                    skinnedShader->set("finalBoneMatrices[" + std::to_string(i) + "]", mats[i]);

                if (auto* mr = entity->getComponent<MeshRendererComponent>()) {
                    if (mr->material) {
                        mr->material->setup();
                        skinnedShader->use();
                    }
                }
                anim->skinnedMesh->draw();
            }
        }

        // If there is a postprocess material, apply postprocessing
        if(postprocessMaterial){
            //TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            //TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();
            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

}