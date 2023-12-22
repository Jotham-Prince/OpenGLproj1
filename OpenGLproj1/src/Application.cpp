#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

// Error Handling

#define ASSERT(x) if(!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
        x;\
        ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError() 
{
    while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError()) 
    {
        std::cout << "[OpenGL error] (" << error << ") " << function << " " << file << line << std::endl;
        return false;
    }
    return true;
}

// A Parser to read in the shader file and split it into the Vertex and Fragment Shaders

struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filepath) 
{
    try 
    {
        std::ifstream stream(filepath);
        if (!stream.is_open())
        {
            throw std::runtime_error("The shader file failed to open");
            return {};
        }

        enum class ShaderType
        {
            NONE = -1, VERTEX = 0, FRAGMENT = 1
        };

        std::string line;
        std::stringstream ss[2];
        ShaderType type = ShaderType::NONE;
        while (getline(stream, line))
        {
            if (line.find("#shader") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    type = ShaderType::VERTEX;
                else if (line.find("fragment") != std::string::npos)
                    type = ShaderType::FRAGMENT;
                else {
                    throw std::runtime_error("Failed to find vertex or fragment shaders!");
                    return {};
                }
            }
            else
            {
                ss[(int)type] << line << '\n';
            }
        }

        return { ss[0].str(), ss[1].str() };
    }
    catch (const std::exception& e)
    {
        std::cerr << "An Error occured: " << e.what() << std::endl;
    }
}

// To compile the Individual Shader Programs

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment ") << "shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

// To create our shader program

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;
    
    window = glfwCreateWindow(640, 480, "THE PENTAGON", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        std::cout << "Error! GLEW wasn't initiallized properly" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl; //This prints the OpenGL version

    // Vertex positions (x, y)
    float positions[] = {
        -0.3f, -0.65f, // 0
        0.3f, -0.65f, // 1
        0.5f, 0.0f, // 2
        0.0f, 0.5f, // 3
        -0.5f, 0.0f // 4
    };

    // Vertex colors (RGB)
    float colors[] = {
        1.0f, 0.0f, 0.0f, // Red
        0.0f, 1.0f, 0.0f, // Green
        0.0f, 0.0f, 1.0f, // Blue
        1.0f, 1.0f, 0.0f, // Yellow
        0.0f, 1.0f, 1.0f  // Cyan
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 4,
        4, 0, 2
    };

    // Setting up the Vertex Buffers for position and colors
    unsigned int positionBuffer, colorBuffer;

    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

    // Setting up the index buffer
    unsigned int ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    glUseProgram(shader);

    //Setup the transformation matrices
    glm::mat4 model = glm::mat4(1.0f); //Initialize the 4x4 model matrix
    glm::mat4 originalModel = glm::mat4(1.0f);

    GLfloat scaleX = 1.0f;
    GLfloat scaleY = 1.0f;
    GLfloat angle = 0.0f;

    glm::mat4 tr = glm::translate(glm::mat4(1.0f), glm::vec3(-0.3f, 0.5f, 0.0f)); // Translation
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotation
    glm::mat4 sc = glm::scale(glm::mat4(1.0f), glm::vec3( scaleX, scaleY, 1.0f)); // Scaling

    //Transformation applied to the model
    //model = rot * sc;

    //Composite Transformation Initialization
    GLuint delayTime = 4000;
    GLuint timer = 50;
    bool Turn = true;

    //Structure of the model matrix
    std::cout << glm::to_string(model[0]) << std::endl;
    std::cout << glm::to_string(model[1]) << std::endl;
    std::cout << glm::to_string(model[2]) << std::endl;
    std::cout << glm::to_string(model[3]) << std::endl;

    // Get the uniform location for the model matrix
    unsigned int modelLoc = glGetUniformLocation(shader, "u_Model");

    // Apply the model matrix to the shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        GLCall(glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, nullptr));

        if (timer == 0) {
            scaleX -= 0.05;
            scaleY -= 0.05;
            angle += 20.0f;

            rot = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotation
            sc = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f)); // Scaling

            if (Turn) {
                model = sc * model;
                Turn = !Turn;
                timer = delayTime;
            }
            else {
                model = rot * model;
                Turn = !Turn;
                timer = delayTime;
            }

            // Apply the model matrix to the shader
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        timer -= 1;
    }

    glDeleteProgram(shader); //Jhus to clean up the program

    glfwTerminate();
    return 0;
}