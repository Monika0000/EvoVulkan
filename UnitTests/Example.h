//
// Created by Nikita on 05.05.2021.
//

#ifndef EVOVULKAN_EXAMPLE_H
#define EVOVULKAN_EXAMPLE_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <EvoVulkan/Types/Texture.h>

#include <stbi.h>

#include <array>

#include <EvoVulkan/VulkanKernel.h>

#include <EvoVulkan/Types/RenderPass.h>

#include <EvoVulkan/Complexes/Shader.h>
#include <EvoVulkan/Complexes/Mesh.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

#include <cmp_core.h>

const std::string resources = "Z:/SREngine/Engine/Core/Dependences/Framework/Depends/EvoVulkan/Resources";

using namespace EvoVulkan;

struct ViewUniformBuffer {
    glm::mat4 projection;
    glm::mat4 view;
};

struct ModelUniformBuffer {
    //glm::mat4 projection;
    //glm::mat4 view;
    glm::mat4 model;
};

struct SkyboxUniformBuffer {
    glm::mat4 proj;
    glm::mat4 view;
    glm::vec3 camPos;
};

struct PPUniformBuffer {
    float gamma;
};

struct VertexUV {
    float position[3];
    float uv[2];
};

struct Vertex {
    float position[3];
};

struct Mesh {
    Core::DescriptorManager* m_descrManager  = nullptr;
    Core::DescriptorSet      m_descriptorSet = {};

    Types::Buffer*           m_uniformBuffer = nullptr;
    Types::Buffer*           m_vertexBuffer  = nullptr;
    Types::Buffer*           m_indexBuffer   = nullptr;
    ModelUniformBuffer       m_ubo           = {};
    uint64_t                 m_countIndices  = 0;

    void Draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout) const {
        if (!m_vertexBuffer || !m_indexBuffer) {
            return;
        }

        VkDeviceSize offsets[1] = {0};

        VkBuffer vertexBuffer = *m_vertexBuffer;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_descriptorSet.m_self, 0, NULL);
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, *m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_countIndices, 1, 0, 0, 0);
    }

    void Destroy() {
        EVSafeFreeObject(m_uniformBuffer);

        m_vertexBuffer = nullptr;
        m_indexBuffer  = nullptr;

        if (m_descriptorSet.m_self != VK_NULL_HANDLE) {
            m_descrManager->FreeDescriptorSet(m_descriptorSet);
            m_descriptorSet.Reset();
        }

        m_descrManager = nullptr;
        m_countIndices = 0;
    }
};

class VulkanExample : public Core::VulkanKernel {
public:
    Complexes::Shader*          m_geometry            = nullptr;
    Complexes::Shader*          m_skyboxShader        = nullptr;

    Types::Buffer*              m_viewUniformBuffer   = nullptr;

    Complexes::Shader*          m_postProcessing      = nullptr;
    Core::DescriptorSet         m_PPDescriptorSet     = { };
    Types::Buffer*              m_PPUniformBuffer     = nullptr;

    Types::Buffer*              m_planeVerticesBuff   = nullptr;
    Types::Buffer*              m_planeIndicesBuff    = nullptr;

    Types::Buffer*              m_skyboxVerticesBuff  = nullptr;
    Types::Buffer*              m_skyboxIndicesBuff   = nullptr;

    Types::Texture*             m_texture             = nullptr;
    Types::Texture*             m_cubeMap             = nullptr;

    Complexes::FrameBuffer*     m_offscreen           = nullptr;

    Mesh meshes[3];
    Mesh skybox;

public:
    Core::RenderResult Render() override {
        if (PrepareFrame() == Core::FrameResult::OutOfDate) {
            m_hasErrors = !ResizeWindow();
            return Core::RenderResult::Success;
        }

        m_submitInfo.commandBufferCount = 1;

        m_submitInfo.pWaitSemaphores    = &m_syncs.m_presentComplete;
        m_submitInfo.pSignalSemaphores  = &m_offscreen->m_semaphore;
        m_submitInfo.pCommandBuffers    = &m_offscreen->m_cmdBuff;
        auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to submit first queue!");
            return Core::RenderResult::Fatal;
        }

        m_submitInfo.pWaitSemaphores    = &m_offscreen->m_semaphore;
        m_submitInfo.pSignalSemaphores  = &m_syncs.m_renderComplete;
        m_submitInfo.pCommandBuffers    = &m_drawCmdBuffs[m_currentBuffer];
        result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to submit second queue!");
            return Core::RenderResult::Fatal;
        }

        if (SubmitFrame() == Core::FrameResult::OutOfDate) {
            m_hasErrors = !ResizeWindow();
        }

        return Core::RenderResult::Success;
    }

    void UpdateUBO() {
        glm::mat4 projectionMatrix = glm::perspective(
                glm::radians(
                        1000.f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                1.f,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
                0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                100.0f             // Far clipping plane. Keep as little as possible.
        );

        /*glm::mat4 view = glm::lookAt(
                glm::vec3(4, 3, 3), // Camera is at (4,3,3), in World Space
                glm::vec3(0, 0, 0), // and looks at the origin
                glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );*/
        static float f = 0.f;
        static float x = 0.f;
        static float y = 0.f;
        static float z = 0.f;

        static float yaw = 0.f;

        glm::mat4 view = glm::mat4(1);
        {
            view = glm::rotate(view,
                     -glm::radians(0.f) // pitch
                    , {1, 0, 0}
            );
            view = glm::rotate(view,
                     -glm::radians(yaw) //yaw
                    , {0, 1, 0}
            );
            view = glm::rotate(view,
                     0.f // roll
                    , {0, 0, 1}
            );
        }

        yaw += 0.1;
        float speed = 0.2;

        if ((GetAsyncKeyState(VK_LEFT) < 0))
            x -= speed;
        if ((GetAsyncKeyState(VK_RIGHT) < 0))
            x += speed;

        if ((GetAsyncKeyState(VK_UP) < 0))
            z += speed;
        if ((GetAsyncKeyState(VK_DOWN) < 0))
            z -= speed;

        if ((GetAsyncKeyState(VK_SPACE) < 0))
            y -= speed;
        if ((GetAsyncKeyState(VK_SHIFT) < 0))
            y += speed;

        ViewUniformBuffer viewUbo = {
                projectionMatrix,
                glm::translate(view, glm::vec3(x, y, z))
        };
        this->m_viewUniformBuffer->CopyToDevice(&viewUbo, sizeof(ViewUniformBuffer));

        int i = 0;
        for (auto & _mesh : meshes) {
            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(i * 2.5, 0, 5 * i));
           // model *= glm::mat4(glm::angleAxis(glm::radians(f), glm::vec3(0, 1, 0)));

            model *= glm::mat4(glm::angleAxis(glm::radians(10.f * (float)i), glm::vec3(0, 1, 0)));

            i++;

            _mesh.m_ubo = { model };

            _mesh.m_uniformBuffer->CopyToDevice(&_mesh.m_ubo, sizeof(ModelUniformBuffer));
        }

        SkyboxUniformBuffer ubo = {
                projectionMatrix,
                view,
                glm::vec3(x, y, z)
        };

        skybox.m_uniformBuffer->CopyToDevice(&ubo, sizeof(SkyboxUniformBuffer));
    }

    void LoadSkybox() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (resources + "/Models/skybox.obj").c_str()))
            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};

                    vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                    vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                    vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

                    vertices.push_back(vertex);
                    indices.push_back(indices.size());
                }
            }

        m_skyboxVerticesBuff = Types::Buffer::Create(
                m_device, m_allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertices.size() * sizeof(VertexUV),
                vertices.data());

        m_skyboxIndicesBuff = Types::Buffer::Create(
                m_device, m_allocator, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indices.size() * sizeof(uint32_t),
                indices.data());

        skybox.m_descriptorSet = m_descriptorManager->AllocateDescriptorSets(m_skyboxShader->GetDescriptorSetLayout(), {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        });
        skybox.m_countIndices = indices.size();
        skybox.m_vertexBuffer = m_skyboxVerticesBuff;
        skybox.m_indexBuffer  = m_skyboxIndicesBuff;
        skybox.m_descrManager = m_descriptorManager;

        skybox.m_uniformBuffer = EvoVulkan::Types::Buffer::Create(
                m_device,
                m_allocator,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                sizeof(SkyboxUniformBuffer));

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                Tools::Initializers::WriteDescriptorSet(skybox.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                        skybox.m_uniformBuffer->GetDescriptorRef()),
                Tools::Initializers::WriteDescriptorSet(skybox.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                        m_cubeMap->GetDescriptorRef()),
        };

        vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
    }

    static int32_t Find4(int32_t i) {
        if (i % 4 == 0)
            return i;
        else
            Find4(i - 1);
        return 0;
    }

    static auto MakeGoodSizes(int32_t w, int32_t h) -> auto {
        return std::pair(Find4(w), Find4(h));
    }

    static uint8_t* ResizeToLess(uint32_t ow, uint32_t oh, uint32_t nw, uint32_t nh, uint8_t* pixels) {
        auto* image = (uint8_t*)malloc(nw * nh * 4);
        uint32_t dw = ow - nw;

        for (uint32_t row = 0; row < nh; row++)
            memcpy(image + (nw * 4 * row), pixels + (dw * 4 * row) + (nw * 4 * row), nw * 4);

        return image;
    }

    static uint8_t* Compress(uint32_t w, uint32_t h, uint8_t* pixels) {
        uint32_t blockCount = (w / 4) * (h / 4);
        auto* cmpBuffer = (uint8_t*)malloc(16 * blockCount * 4);
        for (uint32_t col = 0; col < w / 4; col++) {
            for (uint32_t row = 0; row < h / 4; row++) {
                uint32_t colOffs = col * 16;
                uint32_t rowOffs = row * w;

                //! BC1, BC4 - has 8-byte cmp buffer
                CompressBlockBC1(
                        pixels + colOffs + rowOffs * 16,    // source
                        4 * w,                              // count bytes
                        cmpBuffer + colOffs / 2 + (rowOffs * 4) / 2 // dst
                );

                //! other BC has 16-byte cmp buffer
                /*CompressBlockBC7(
                        pixels + colOffs + rowOffs * 16,    // source
                        4 * w,                              // count bytes
                        cmpBuffer + colOffs+ (rowOffs * 4) // dst
                );*/
            }
        }

        return cmpBuffer;
    }

    bool LoadCubeMap() {
        //-512x512
        int w, h, channels;
        std::array<uint8_t*, 6> sides {
                stbi_load((resources + "/Skyboxes/Sea/front.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
                stbi_load((resources + "/Skyboxes/Sea/back.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
                stbi_load((resources + "/Skyboxes/Sea/top.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
                stbi_load((resources + "/Skyboxes/Sea/bottom.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
                stbi_load((resources + "/Skyboxes/Sea/right.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
                stbi_load((resources + "/Skyboxes/Sea/bottom.jpg").c_str(), &w, &h, &channels, STBI_rgb_alpha),
        };

        m_cubeMap = Types::Texture::LoadCubeMap(m_device, m_allocator, m_cmdPool, VK_FORMAT_R8G8B8A8_UNORM, w, h, sides, 1);

        for (auto&& img : sides)
            stbi_image_free(img);

        return m_cubeMap;
    }

    bool LoadTexture() {
        int w, h, channels;
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\Miku\miku.jpeg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\DDLC\Monika\An exception has occured.jpg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\DDLC\Monika\monika_window.jpg)", &w, &h, &channels, STBI_rgb_alpha);
        uint8_t* pixels = stbi_load(R"(J:\C++\EvoVulkan\Resources\Textures\brickwall2.jpg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\DDLC\Monika\An exception has occured.jpg)", &w, &h, &channels, STBI_rgb);
        //uint8_t* pixels = stbi_load(R"(C:\Users\Nikita\AppData\Roaming\Skype\live#3arotaru5craft\media_messaging\emo_cache_v2\^8E63A254ED2714DA468002A2A13DCACFD605BB67B103217D55^pwin10_80_distr.png)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\akira-(been0328)-Anime-kaguya-sama-wa-kokurasetai-_tensai-tachi-no-renai-zunousen_-Shinomiya-Kaguya-5003254.jpeg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\Miku\5UyhDcR0p8g.jpg)", &w, &h, &channels, STBI_rgb_alpha);

        /*uint32_t blockCount = (w / 4) * (h / 4);
        auto* cmpBuffer = (uint8_t*)malloc(16 * blockCount * 4);
        for (uint32_t col = 0; col < w / 4; col++) {
            for (uint32_t row = 0; row < h / 4; row++)
                CompressBlockBC7(pixels + col * 16 + row * 16 * w, 4 * w, cmpBuffer + (col * 16) + (row * w * 4));
        }*/

        if (!pixels) {
            VK_ERROR("Example::LoadTexture() : failed to load texture! Reason: " + std::string(stbi_failure_reason()));
            return false;
        }

        if (auto sz = MakeGoodSizes(w, h); sz != std::pair(w, h)) {
            auto resized = ResizeToLess(w, h, sz.first, sz.second, pixels);
            stbi_image_free(pixels);
            pixels = resized;
            w = sz.first;
            h = sz.second;
        }

        //auto cmpBuffer = Compress(w, h, pixels);

        //S3TC_DXT1
        //for (uint32_t i = 0; i < 40; i++)
        m_texture = Types::Texture::LoadWithoutMip(m_device, m_allocator, m_descriptorManager, m_cmdPool, pixels, VK_FORMAT_R8G8B8A8_SRGB, w, h, VK_FILTER_LINEAR);
        //m_texture = Types::Texture::LoadWithoutMip(m_device, m_allocator, m_descriptorManager, m_cmdPool, cmpBuffer, VK_FORMAT_R8G8B8A8_SRGB, w, h, VK_FILTER_LINEAR);
        //m_texture = Types::Texture::LoadWithoutMip(m_device, m_allocator, m_descriptorManager, m_cmdPool, cmpBuffer, VK_FORMAT_BC1_RGBA_SRGB_BLOCK, w, h, VK_FILTER_LINEAR);
           // m_texture = Types::Texture::LoadWithoutMip(m_device, m_cmdPool, pixels, VK_FORMAT_BC7_UNORM_BLOCK, w, h);
            //m_texture = Types::Texture::LoadCompressed(m_device, m_descriptorManager, m_cmdPool, pixels, VK_FORMAT_BC1_RGB_UNORM_BLOCK, w, h);
            //m_texture = Types::Texture::LoadAutoMip(m_device, m_cmdPool, pixels, VK_FORMAT_R8G8B8A8_SRGB, w, h, 3);

        if (!m_texture)
            return false;

        //free(cmpBuffer);

        return true;
    }

    bool UpdatePP() {
        auto colors = m_offscreen->AllocateColorTextureReferences();

        //auto attach0 = m_offscreen->GetImageDescriptors()[0];
        //auto attach1 = m_offscreen->GetImageDescriptors()[1];

        /*VkDescriptorImageInfo attach0;
        attach0.imageView   = m_texture->m_view;			// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
        attach0.sampler     = m_texture->m_sampler;		// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
        attach0.imageLayout = m_texture->m_imageLayout;	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)
        auto attach1 = attach0;*/

        std::vector<VkWriteDescriptorSet> writeDescriptorSets1 = {
                // Binding 0 : Fragment shader uniform buffer
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                        m_PPUniformBuffer->GetDescriptorRef()),

                // Binding 1: Fragment shader sampler
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                        colors[0]->GetDescriptorRef()),

        };

        std::vector<VkWriteDescriptorSet> writeDescriptorSets2 = {
                // Binding 2: Fragment shader sampler
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                        colors[1]->GetDescriptorRef())
        };


        vkUpdateDescriptorSets(*m_device, writeDescriptorSets1.size(), writeDescriptorSets1.data(), 0, NULL);
        vkUpdateDescriptorSets(*m_device, writeDescriptorSets2.size(), writeDescriptorSets2.data(), 0, NULL);

        return true;
    }

    bool SetupUniforms() {
        m_viewUniformBuffer = EvoVulkan::Types::Buffer::Create(
                m_device,
                m_allocator,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                sizeof(ViewUniformBuffer));

        // geometry

        for (auto&& mesh : meshes) {
            mesh.m_descrManager = m_descriptorManager;

            mesh.m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(m_geometry->GetDescriptorSetLayout(), {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            });
            if (mesh.m_descriptorSet.m_self == VK_NULL_HANDLE) {
                VK_ERROR("VulkanExample::SetupDescriptors() : failed to allocate descriptor sets!");
                return false;
            }

            //!=============================================================================================================

            mesh.m_uniformBuffer = EvoVulkan::Types::Buffer::Create(
                    m_device,
                    m_allocator,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                    sizeof(ModelUniformBuffer));

            // Setup a descriptor image info for the current texture to be used as a combined image sampler
            auto textureDescriptor = m_texture->GetDescriptorRef();

            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    // Binding 0 : Vertex shader uniform buffer
                    Tools::Initializers::WriteDescriptorSet(mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                            mesh.m_uniformBuffer->GetDescriptorRef()),

                    Tools::Initializers::WriteDescriptorSet(mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                                            m_viewUniformBuffer->GetDescriptorRef()),

                    //Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                    //                                        textureDescriptor)
            };
            vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
        }

        /// post processing
        m_PPDescriptorSet = m_descriptorManager->AllocateDescriptorSets(m_postProcessing->GetDescriptorSetLayout(), {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        });

        m_PPUniformBuffer = EvoVulkan::Types::Buffer::Create(
                m_device,
                m_allocator,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                sizeof(PPUniformBuffer));

        return true;
    }

    bool SetupShader() {
        bool hasErrors = false;

        m_geometry = new Complexes::Shader(GetDevice(), m_offscreen->GetRenderPass(), GetPipelineCache());

        hasErrors |= !m_geometry->Load("J:/C++/EvoVulkan/Resources/Cache",
                         {
                                 { "geometry.vert", resources + "/Shaders/geometry.vert", VK_SHADER_STAGE_VERTEX_BIT },
                                 { "geometry.frag", resources + "/Shaders/geometry.frag", VK_SHADER_STAGE_FRAGMENT_BIT },
                         },
                         {
                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                 VK_SHADER_STAGE_VERTEX_BIT, 0),

                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                 VK_SHADER_STAGE_VERTEX_BIT, 1),

                                 Tools::Initializers::DescriptorSetLayoutBinding(
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                         },
                         {
                                 sizeof(ModelUniformBuffer), sizeof(ViewUniformBuffer)
                         });

        hasErrors |= !m_geometry->SetVertexDescriptions(
                {Tools::Initializers::VertexInputBindingDescription(0, sizeof(VertexUV), VK_VERTEX_INPUT_RATE_VERTEX)},
                {
                        Tools::Initializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                                                             offsetof(VertexUV, position)),
                        Tools::Initializers::VertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,
                                                                             offsetof(VertexUV, uv))
                }
        );

        hasErrors |= !m_geometry->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE,
                VK_TRUE,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        );

        //!=============================================================================================================

        m_skyboxShader = new Complexes::Shader(GetDevice(), m_offscreen->GetRenderPass(), GetPipelineCache());

        hasErrors |= !m_skyboxShader->Load("J:/C++/EvoVulkan/Resources/Cache",
                         {
                                 {"skybox.vert", resources + "/Shaders/skybox.vert", VK_SHADER_STAGE_VERTEX_BIT},
                                 {"skybox.frag", resources + "/Shaders/skybox.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                         },
                         {
                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                 VK_SHADER_STAGE_VERTEX_BIT, 0),

                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                         },
                         {
                                 sizeof(SkyboxUniformBuffer)
                         });

        hasErrors |= !m_skyboxShader->SetVertexDescriptions(
                {Tools::Initializers::VertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)},
                {
                        Tools::Initializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                                                             offsetof(Vertex, position)),
                }
        );

        hasErrors |= !m_skyboxShader->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE,
                VK_TRUE,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        );

        //!=============================================================================================================

        m_postProcessing = new Complexes::Shader(GetDevice(), this->GetRenderPass(), GetPipelineCache());

        hasErrors |= !m_postProcessing->Load("J://C++/EvoVulkan/Resources/Cache",
                               {
                                       {"post_processing.vert", resources + "/Shaders/post_processing.vert", VK_SHADER_STAGE_VERTEX_BIT},
                                       {"post_processing.frag", resources + "/Shaders/post_processing.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                               },
                               {
                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 0),

                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 1),

                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                               },
                               {
                                       sizeof(PPUniformBuffer)
                               });

        hasErrors |= !m_postProcessing->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE,
                VK_TRUE,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        );

        return !hasErrors;
    }

    bool GenerateGeometry() {
        // Setup vertices for a single uv-mapped quad made from two triangles
        std::vector<VertexUV> vertices =
                {
                        { {2.0f,  2.0f,  0.0f}, { 1.0f, 1.0f } },
                        { {-2.0f, 2.0f,  0.0f}, { 0.0f, 1.0f } },
                        { {-2.0f, -2.0f, 0.0f}, { 0.0f, 0.0f } },
                        { {2.0f,  -2.0f, 0.0f}, { 1.0f, 0.0f } }
                };

        /// Setup indices
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        /// Create buffers
        /// For the sake of simplicity we won't stage the vertex data to the gpu memory
        /// Vertex buffer
        m_planeVerticesBuff = Types::Buffer::Create(
                m_device,
                m_allocator,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertices.size() * sizeof(VertexUV),
                vertices.data());

        /// Index buffer
        m_planeIndicesBuff = Types::Buffer::Create(
                m_device,
                m_allocator,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indices.size() * sizeof(uint32_t),
                indices.data());

        for (auto&& mesh : meshes) {
            mesh.m_indexBuffer  = m_planeIndicesBuff;
            mesh.m_vertexBuffer = m_planeVerticesBuff;
            mesh.m_countIndices = indices.size();
        }

        return true;
    }

    bool BuildCmdBuffers123123123() {
        VkCommandBufferBeginInfo cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

        VkClearValue clearValues[2] {
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };

        auto renderPassBI = Tools::Insert::RenderPassBeginInfo(
                m_width, m_height, m_renderPass.m_self,
                VK_NULL_HANDLE, &clearValues[0], 2);

        for (int i = 0; i < 3; i++) {
            renderPassBI.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Tools::Initializers::Viewport((float) m_width, (float) m_height, 0.0f, 1.0f);
            vkCmdSetViewport(m_drawCmdBuffs[i], 0, 1, &viewport);

            VkRect2D scissor = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);
            vkCmdSetScissor(m_drawCmdBuffs[i], 0, 1, &scissor);

            vkCmdBindPipeline(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_geometry);

            for (auto &_mesh : meshes)
                _mesh.Draw(m_drawCmdBuffs[i], m_geometry->GetPipelineLayout());

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }

    bool BuildCmdBuffers() override {
        std::vector<VkClearValue> clearValues = {
                VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // color
                VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // resolve

                VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // color
                VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // resolve

                VkClearValue{ .depthStencil = { 1.0f, 0 } } // depth
        };

        if (!m_multisampling) {
            clearValues = {
                    VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // color
                    VkClearValue{ .color = {{0.0f, 0.0f, 0.0f, 1.0f}} }, // color

                    VkClearValue{ .depthStencil = { 1.0f, 0 } } // depth
            };
        }

        VkRenderPassBeginInfo renderPassBeginInfo = m_offscreen->BeginRenderPass(&clearValues[0], clearValues.size());

        m_offscreen->BeginCmd();
        vkCmdBeginRenderPass(m_offscreen->GetCmd(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        m_offscreen->SetViewportAndScissor();

        for (auto & _mesh : meshes) {
            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                            m_texture->GetDescriptorRef())
            };
            vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
        }

        {
            vkCmdBindPipeline(m_offscreen->GetCmd(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_geometry);

            for (auto & _mesh : meshes)
                _mesh.Draw(m_offscreen->GetCmd(), m_geometry->GetPipelineLayout());
        }

        {
            vkCmdBindPipeline(m_offscreen->m_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_skyboxShader);

            skybox.Draw(m_offscreen->m_cmdBuff, m_skyboxShader->GetPipelineLayout());
        }

        m_offscreen->End();

        UpdatePP();

        return BuildCmdBuffersPostProcess();
    }

    bool BuildCmdBuffersPostProcess() {
        VkCommandBufferBeginInfo cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

        VkClearValue clearValues[3] {
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };

        auto renderPassBI = Tools::Insert::RenderPassBeginInfo(
                m_width, m_height, m_renderPass.m_self,
                VK_NULL_HANDLE, &clearValues[0], 3);

        for (int i = 0; i < m_frameBuffers.size(); ++i) {
            renderPassBI.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
            vkCmdSetViewport(m_drawCmdBuffs[i], 0, 1, &viewport);

            VkRect2D scissor = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);
            vkCmdSetScissor(m_drawCmdBuffs[i], 0, 1, &scissor);

            vkCmdBindPipeline(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_postProcessing);
            vkCmdBindDescriptorSets(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessing->GetPipelineLayout(), 0, 1, &m_PPDescriptorSet.m_self, 0, NULL);

            vkCmdDraw(m_drawCmdBuffs[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }

    bool Destroy() override {
        VK_LOG("Example::Destroy() : destroy kernel inherit class...");

        EVSafeFreeObject(m_offscreen);

        EVSafeFreeObject(m_texture);
        EVSafeFreeObject(m_cubeMap);

        EVSafeFreeObject(m_PPUniformBuffer);
        if (m_PPDescriptorSet.m_self != VK_NULL_HANDLE) {
            m_descriptorManager->FreeDescriptorSet(m_PPDescriptorSet);
            m_PPDescriptorSet.Reset();
        }

        EVSafeFreeObject(m_viewUniformBuffer);

        EVSafeFreeObject(m_geometry);
        EVSafeFreeObject(m_postProcessing);
        EVSafeFreeObject(m_skyboxShader);

        for (auto&& mesh : meshes)
            mesh.Destroy();
        skybox.Destroy();

        EVSafeFreeObject(m_skyboxVerticesBuff);
        EVSafeFreeObject(m_skyboxIndicesBuff);

        EVSafeFreeObject(m_planeVerticesBuff);
        EVSafeFreeObject(m_planeIndicesBuff);

        return VulkanKernel::Destroy();
    }

    bool OnComplete() override {
        m_offscreen = Complexes::FrameBuffer::Create(
                m_device,
                m_allocator,
                m_descriptorManager,
                m_swapchain,
                m_cmdPool,
                {
                        VK_FORMAT_R8G8B8A8_UNORM,
                        VK_FORMAT_R8G8B8A8_UNORM,
                        //this->m_swapchain->GetColorFormat(),
                        //this->m_swapchain->GetColorFormat(),
                        //VK_FORMAT_R32G32B32A32_SFLOAT,
                        //VK_FORMAT_R32G32B32A32_SFLOAT,
                        //VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT
                },
                m_width,
                m_height,
                1.f);

        if (!m_offscreen)
            return false;

        return true;
    }

    bool OnResize() override {
        vkQueueWaitIdle(m_device->GetGraphicsQueue());
        vkDeviceWaitIdle(*m_device);

        return m_offscreen ? (m_offscreen->ReCreate(m_width, m_height) && UpdatePP()) : true;
    }
};

#endif //EVOVULKAN_EXAMPLE_H
