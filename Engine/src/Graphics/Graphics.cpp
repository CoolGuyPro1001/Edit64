#include "Graphics.h"

#include "GLDebug.h"
#include "File.h"
#include "Texture.h"
#include "Core/Log.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/projection.hpp>


#define NULL_ID UINT_MAX
namespace Graphics
{
    //static std::vector<VertexBuffer*> vertex_buffers = std::vector<VertexBuffer*>();
    //static std::vector<IndexBuffer*> index_buffers = std::vector<IndexBuffer*>();


    ///@brief Parses a .shader file into a Shader object
    ///@param file_path The full file path to the shader file
    ///@return Shader Object

    uint transfrom_uniform_id = NULL_ID;
    uint shader_program = NULL_ID;
    bool initialized = false;

    uint buffer_id = NULL_ID;
    uint vao_id = NULL_ID;

    SDL_Window* window;
    int window_width = 4;
    int window_height = 3;

    static void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error: %s\n", description);
    }
    
    uint* BufferId()
    {
        return &buffer_id;
    }

    uint* VaoId()
    {
        return &vao_id;
    }

    Shader ParseShader(const std::string& file_path)
    {
        enum ShaderType
        {
            NONE= -1, VERTEX = 0, FRAGMENT = 1
        };

        const std::string shader_source = File::ReadFile(file_path);
        Shader shader_source_parsed;
        ShaderType current_type = ShaderType::NONE;

        int begin = 0; //The beginning of substring
        int end = 0; //The end of substring current marker
        while(true)
        {
            //Move end to closest #shader
            end = shader_source.find("#shader", end);
            if(end == std::string::npos)
            {
                end = shader_source.length() - 1;
            }

            //Set ShaderSource based on previous block of code
            switch(current_type)
            {
                case ShaderType::VERTEX:
                    shader_source_parsed.vertex_source = shader_source.substr(begin, (end - begin));
                    break;
                case ShaderType::FRAGMENT:
                    shader_source_parsed.fragment_source = shader_source.substr(begin, (end - begin));
                    break;
                default:
                    break;
            }

            //Break when end reaches the end of shader source code text
            if(end == shader_source.length() - 1)
                break;

            //Find out what current block of code is for
            begin = end;
            if(shader_source.substr((end + 8), 6) == "vertex")
            {
                current_type = ShaderType::VERTEX;
                begin += 15;
            }
            else if(shader_source.substr((end + 8), 8) == "fragment")
            {
                current_type = ShaderType::FRAGMENT;
                begin += 17;
            }

            end++;
        }

        return shader_source_parsed;
    }

    ///@brief Initializes OpenGL functions
    ///@return True if initialization is successful
    bool Initialize()
    {
        GLenum error = glewInit();
        if(GLEW_OK != error)
        {
            Engine::Log("ERROR: GLEW failed to intialize\n%s", (const char*)glewGetErrorString(error));
            return false;
        }
        
        
        glGenVertexArrays(1, &vao_id);
        glBindVertexArray(vao_id);
        
        initialized = true;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        return true;
    }

    bool InitWindow(int width, int height, const char* name)
    {
        window = SDL_CreateWindow
        (
            name, 
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED, 
            width, 
            height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
        );

        if(!window)
        {
            return false;
        }

        SDL_GL_SetSwapInterval(1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_CreateContext(window);
        return true;
    }

    void CloseWindow()
    {
        if(window)
        {
            SDL_DestroyWindow(window);
        }
    }

    ///@brief Uses the shader program from .shader file
    ///@param shader_file_path The full path to the shader file
    void UseShader(const std::string shader_file_path)
    {
        Shader shader = ParseShader(shader_file_path);
        shader_program = shader.CreateProgram();
        glUseProgram(shader_program);
    }

    glm::mat4 TransformationMatrix(Vector3 position, Vector3 rotation, Vector3 scale)
    {
        glm::mat4 rotation_matrix =    glm::rotate(rotation.pitch, glm::vec3(1.0f, 0.0f, 0.0f)) * 
                                glm::rotate(rotation.yaw, glm::vec3(0.0f, 1.0f, 0.0f)) * 
                                glm::rotate(rotation.roll, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 translate_matrix = glm::translate(glm::vec3(position.x, position.y, position.z));
        glm::mat4 scale_matrix = glm::scale(glm::vec3(scale.x, scale.y, scale.z));
        
        return translate_matrix * scale_matrix * rotation_matrix;
    }

    glm::vec3 Vector3GLM(const Vector3 v)
    {
        return glm::vec3(v.x, v.y, v.z);
    }

    ///@brief Draws To Screen
    void Draw(std::map<Shared<Model>, std::vector<Shared<Object>>>& instances ,Shared<Camera> camera, Shared<Model> sky_block, Shared<Model> terrain)
    {
        if(!initialized) //|| shader_program == NULL_ID)
        {
            return;
        }

        //Setup
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(vao_id);

        //Create Matrix For 3D View
        glm::mat4 projection = glm::perspective(glm::radians(camera->fov), (float) window_width / (float) window_height, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(Vector3GLM(camera->position), Vector3GLM(camera->looking_at), Vector3GLM(camera->up));
        glm::mat4 to_3d = projection * view;

        int offset = 0;
        int mvps_id = glGetUniformLocation(shader_program, "mvps");

        //Draw Sky Block (Offset 0)
        glm::mat4 sky_block_mvp = to_3d * TransformationMatrix(Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1));
        glUniformMatrix4fv(mvps_id, 1, GL_FALSE, &sky_block_mvp[0][0]);
        sky_block->texture->Use(shader_program);
        glDrawArraysInstanced(GL_TRIANGLES, offset, sky_block->vertices.size(), 1);
        offset += sky_block->vertices.size();
        
        //Draw Terrain Block
        glm::mat4 terrain_mvp = to_3d * TransformationMatrix(Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1));
        glUniformMatrix4fv(mvps_id, 1, GL_FALSE, &terrain_mvp[0][0]);
        terrain->texture->Use(shader_program);
        glDrawArraysInstanced(GL_TRIANGLES, offset, terrain->vertices.size(), 1);
        offset += terrain->vertices.size();

        //Draw Actors
        for (auto const& [model, objects] : instances)
        {
            std::vector<glm::mat4> mvps = std::vector<glm::mat4>();
            
            for(Shared<Object> object : objects)
                mvps.push_back(to_3d * TransformationMatrix(object->position, object->rotation, object->scale));

            if(model->texture)
            {
                model->texture->Use(shader_program);
            }
            
            glUniformMatrix4fv(mvps_id, objects.size(), GL_FALSE, &mvps[0][0][0]);

            uint size = model->vertices.size();
            uint index = model->offset / sizeof(Vertex);
            glDrawArraysInstanced(GL_TRIANGLES, index, size, objects.size());
        }
        
        if(window)
        {
            SDL_GL_SwapWindow(window);
        }
    }
}