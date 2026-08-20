#include <GL/glew.h> // must come before glfw3.h
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // needed for glm::value_ptr
#include <iostream>
#include <vector>
 
#include "objloader.h"


// ---------------------------------------------------------------------------
// Configuration constants
// ---------------------------------------------------------------------------
const int   WINDOW_WIDTH  = 800;
const int   WINDOW_HEIGHT = 600;
 
const float MOVE_DISTANCE = 1.0f; // "d": distance travelled per second while a key is held
const float ROTATION_STEP = 30.0f; // degrees per key press
const float SCALE_FACTOR  = 1.1f; // "s": multiplied / divided per key press
 
const char* MODEL_PATH = "Chair.obj";   
 

// ---------------------------------------------------------------------------
// Shader sources
// ---------------------------------------------------------------------------
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;

    void main() {
        FragColor = vec4(0.9, 0.9, 0.9, 1.0);
    }
)glsl";

// ---------------------------------------------------------------------------
// Holds the current state of the model and builds its model matrix.
// Keeping the state in one place makes the render loop and the input handling
// easy to read, and guarantees the transformations are always composed in the
// same order (translate -> rotate -> scale).
// ---------------------------------------------------------------------------
struct ModelTransform {
    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
    float rotationZ = 0.0f; // degrees around the z axis
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 center = glm::vec3(0.0f);
    float fitScale = 1.0f;
 
    glm::mat4 modelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale * fitScale);
        model = glm::translate(model, -center);
        return model;
    }
};



// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------
 
// Keeps the rendering area in sync with the window if the user resizes it.
void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}
 
// Compiles one shader stage and reports any compilation error.
// Returns 0 on failure.
unsigned int compileShader(GLenum shaderType, const char* source) {
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
 
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


// Compiles both stages and links them into a shader program.
// Returns 0 on failure.
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return 0;
 
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }
 
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
 
    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << "\n";
        glDeleteProgram(program);
        program = 0;
    }
 
    // The individual shader objects are no longer needed once linked.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Returns true only on the frame where a key goes from released to pressed.
// Rotation and scaling need this: glfwGetKey() reports GLFW_PRESS on every
// frame the key is held down, so without this edge detection a single tap
// would apply 30 degrees (or a 1.1x scale) dozens of times in a fraction of
// a second.
bool keyJustPressed(GLFWwindow* window, int key) {
    static bool wasDown[GLFW_KEY_LAST + 1] = { false };
 
    bool isDown      = (glfwGetKey(window, key) == GLFW_PRESS);
    bool justPressed = isDown && !wasDown[key];
    wasDown[key]     = isDown;
    return justPressed;
}


// ---------------------------------------------------------------------------
// Keyboard input.
// Called once per frame. deltaTime is the duration of the previous frame in
// seconds, which makes the translation speed independent of the frame rate.
// ---------------------------------------------------------------------------
void processInput(GLFWwindow* window, ModelTransform& obj, float deltaTime) {
    // ESC closes the window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
 
    // --- Translation: continuous while the key is held ---
    const float step = MOVE_DISTANCE * deltaTime;
 
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        obj.translation.y += step;   // up
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        obj.translation.y -= step;   // down
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        obj.translation.x -= step;   // left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        obj.translation.x += step;   // right
 
    // --- Rotation around the z axis: one 30 degree step per press ---
    if (keyJustPressed(window, GLFW_KEY_Q))
        obj.rotationZ += ROTATION_STEP;   // anticlockwise (positive angle, right-hand rule)
    if (keyJustPressed(window, GLFW_KEY_E))
        obj.rotationZ -= ROTATION_STEP;   // clockwise
 
    // Keep the angle in [0, 360) so it never grows without bound.
    if (obj.rotationZ >= 360.0f) obj.rotationZ -= 360.0f;
    if (obj.rotationZ <    0.0f) obj.rotationZ += 360.0f;
 
    // --- Scaling along the z axis: one factor s per press ---
    if (keyJustPressed(window, GLFW_KEY_R))
        obj.scale.z *= SCALE_FACTOR;      // stretch along z, symmetric about the model's centre
    if (keyJustPressed(window, GLFW_KEY_F))
        obj.scale.z /= SCALE_FACTOR;      // shrink along z, exactly undoing one R press
}
 



int main() {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
    if (!loadOBJ(MODEL_PATH, positions, indices)) {
        return -1;
    }

    glm::vec3 minV = positions[0], maxV = positions[0];
    for (const glm::vec3& p : positions) {
        minV = glm::min(minV, p);
        maxV = glm::max(maxV, p);
    }
    glm::vec3 center = (minV + maxV) * 0.5f;
    glm::vec3 size   = maxV - minV;
    float fitScale   = 2.0f / glm::max(size.x, glm::max(size.y, size.z));

    std::cout << "positions: " << positions.size()
              << "  indices: " << indices.size() << "\n";

    // ---- Window and OpenGL context ----
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "COMP 371 - A3", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
 
    glfwMakeContextCurrent(window);   // context must exist before glewInit
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
 
    // GLEW queries the driver for the OpenGL function pointers
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        glfwTerminate();
        return -1;
    }
 
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
 
    // Wireframe: draw edges only, front and back faces alike.
    // Face culling stays off (the default) so the far edges remain visible.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
 
    // ---- Shaders ----
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) {
        glfwTerminate();
        return -1;
    }
 
    // ---- VAO / VBO / EBO ----
    // The geometry now comes from the vectors filled by loadOBJ(). A vector's
    // contents live on the heap, so sizeof() would measure the vector object
    // rather than the data: use .size() * sizeof(element) and .data() instead.
    const GLsizei INDEX_COUNT = static_cast<GLsizei>(indices.size());
 
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
 
    glBindVertexArray(VAO);
 
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 positions.size() * sizeof(glm::vec3),
                 positions.data(),
                 GL_STATIC_DRAW);
 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);
 
    // Positions only now, tightly packed: stride is one vec3, offset is zero.
    const int STRIDE = sizeof(glm::vec3);
 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
 
    glBindVertexArray(0);   // unbind, good practice
 
    // ---- Camera and projection ----
    glm::mat4 view = glm::lookAt(
        glm::vec3(2.0f, 1.5f, 5.0f),   // eye
        glm::vec3(0.0f, 0.0f, 0.0f),   // looking at the origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // +y is up
    );
 
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),                                        // field of view
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.1f,                                                       // near plane
        100.0f                                                      // far plane
    );
 
    // ---- Uniform locations (they never change, so look them up once) ----
    // These must be signed ints: glGetUniformLocation returns -1 on failure,
    // and storing -1 in an unsigned int hides the error behind a huge number.
    int modelLoc      = glGetUniformLocation(shaderProgram, "model");
    int viewLoc       = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    if (modelLoc < 0 || viewLoc < 0 || projectionLoc < 0) {
        std::cerr << "WARNING: a uniform was not found in the shader program\n";
    }
 
    // The view and projection matrices are constant, so upload them once.
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
 
    // ---- Live state driven by the keyboard ----
    ModelTransform obj;
    obj.center = center;
    obj.fitScale = fitScale;
    float lastFrameTime = static_cast<float>(glfwGetTime());
 
    // ---- Render loop ----
    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime   = currentTime - lastFrameTime;
        lastFrameTime     = currentTime;

        // Read the keyboard and update the chair's state
        processInput(window, obj, deltaTime);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Build the model matrix from the current state and send it
        // to the "model" uniform in the vertex shader
        glm::mat4 model = obj.modelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);   // double buffering
        glfwPollEvents();          // process input events
    }
 
    // ---- Cleanup ----
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
 
    glfwTerminate();
    return 0;
}