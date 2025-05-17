/* Hello Triangle - code adapted from https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adapted by Rossana Baptista Queiroz
 * for the disciplines of Computer Graphics - Unisinos
 * Initial version: 7/4/2017
 * Last update: 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <vector> // Include vector header
#include <random> // For random cube positions
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Random number generator for cube positions
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(-3.0, 3.0); // Random position between -3 and 3


// Keyboard callback function prototype
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Function prototypes
int setupShader();
int setupGeometry();

// Window dimensions (can be changed at runtime)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Vertex Shader source code (in GLSL): still hardcoded
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = projection * view * model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

// Fragment Shader source code (in GLSL): still hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

// Structure to hold OBJ model data and transformations
struct OBJModel {
    GLuint VAO;
    int numVertices;
    glm::vec3 position;
    glm::vec3 rotation; // Euler angles for simplicity
    glm::vec3 scale;
    std::vector<glm::vec3> vertices; // Store vertex positions for intersection testing

    OBJModel(GLuint vao, int vertices) : VAO(vao), numVertices(vertices), position(0.0f), rotation(0.0f), scale(1.0f) {}
};

// Function to load a simple OBJ file (copied from LoadSimpleOBJ.cpp)
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 color, std::vector<glm::vec3>& outVertices)
 {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    // glm::vec3 color = glm::vec3(1.0, 0.0, 0.0); // Default color // Remove this line

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
	{
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
		{
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
		{
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
		{
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
		 {
            while (ssline >> word)
			{
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }

    arqEntrada.close();

    outVertices = vertices; // Store vertices in the output vector

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	nVertices = vBuffer.size() / 6;  // x, y, z, r, g, b (valores atualmente armazenados por vértice)

    return VAO;
}


bool rotateX=false, rotateY=false, rotateZ=false;
// float translateX = 0.0f, translateY = 0.0f, translateZ = 0.0f; // Remove temporary translation variables
// float scale = 1.0f; // Remove temporary scale variable

bool isMovingForward = false, isMovingBackward = false, isMovingLeft = false, isMovingRight = false, isMovingUp = false, isMovingDown = false;
bool isScalingUp = false, isScalingDown = false;

// Transformation speeds
const float translationSpeed = 2.5f; // Units per second
const float rotationSpeed = 50.0f; // Degrees per second
const float scalingSpeed = 1.1f; // Scaling factor per second

// Use a vector for dynamic cube offsets
// std::vector<glm::vec3> cubeOffsets; // Remove this line
std::vector<OBJModel> models; // Use a vector for OBJ models
size_t selectedModelIndex = 0; // Index of the currently selected model

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

// Function to test for ray-triangle intersection (Moller-Trumbore algorithm)
bool intersectTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& outDistance)
{
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1, edge2, h, s, q;
    float a, f, u, v;

    edge1 = v1 - v0;
    edge2 = v2 - v0;

    h = glm::cross(rayDir, edge2);
    a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to the triangle

    f = 1.0f / a;
    s = rayOrigin - v0;
    u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    q = glm::cross(s, edge1);
    v = f * glm::dot(rayDir, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    // At this stage, we can compute t to find out where the intersection point is on the line.
    float t = f * glm::dot(edge2, q);

    if (t > EPSILON) // ray intersects the triangle
    {
        outDistance = t;
        return true;
    }

    // This means that the intersection point was behind the ray
    return false;
}

// MAIN function
int main()
{
	// GLFW initialization
	glfwInit();

	//Muita atenção aqui: alguns ambientes não aceitam essas configurações
	//Você deve adaptar para a versão do OpenGL suportada por sua placa
	//Sugestão: comente essas linhas de código para desobrir a versão e
	//depois atualize (por exemplo: 4.5 com 4 e 5)
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Essencial para computadores da Apple
//#ifdef __APPLE__
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif

	// Create GLFW window
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D – Otavio!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Register keyboard callback function
	glfwSetKeyCallback(window, key_callback);
    // Register mouse button callback function
    glfwSetMouseButtonCallback(window, mouse_button_callback);

	// GLAD: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Get version information
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Define viewport dimensions with the same dimensions as the application window
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compile and build the shader program
	GLuint shaderID = setupShader();

	// Generate a simple buffer with triangle geometry
	// GLuint VAO = setupGeometry(); // Remove this line

    // Initial cube offset - start with just one cube at the origin
    // cubeOffsets.push_back(glm::vec3(0.0f, 0.0f, 0.0f)); // Remove this line

    // Load OBJ models
    int numVerticesSuzanne;
    std::vector<glm::vec3> verticesSuzanne;
    GLuint suzanneVAO = loadSimpleOBJ("../../assets/Modelos3D/Suzanne.obj", numVerticesSuzanne, glm::vec3(1.0f, 0.0f, 0.0f), verticesSuzanne); // Red color
    if (suzanneVAO != -1) {
        models.push_back(OBJModel(suzanneVAO, numVerticesSuzanne));
        models.back().vertices = verticesSuzanne;
    }

    // Load another Suzanne model
    int numVerticesSuzanne2;
    std::vector<glm::vec3> verticesSuzanne2;
    GLuint suzanneVAO2 = loadSimpleOBJ("../../assets/Modelos3D/Suzanne.obj", numVerticesSuzanne2, glm::vec3(1.0f, 1.0f, 0.0f), verticesSuzanne2); // Yellow color
    if (suzanneVAO2 != -1) {
        models.push_back(OBJModel(suzanneVAO2, numVerticesSuzanne2));
        models.back().vertices = verticesSuzanne2;
    }


	glUseProgram(shaderID);

	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	GLint projLoc = glGetUniformLocation(shaderID, "projection");

	glEnable(GL_DEPTH_TEST);

    // Time variables for smooth movement
    float lastFrame = 0.0f;

	// Application loop - "game loop"
	while (!glfwWindowShouldClose(window))
	{
        // Calculate delta time
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		glfwPollEvents();

        // Calculate view matrix (camera) - simple example
        viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), // Camera position
                                     glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // Up vector

        // Calculate projection matrix - perspective projection
        projectionMatrix = glm::perspective(glm::radians(45.0f), // Field of view
                                                (GLfloat)WIDTH / (GLfloat)HEIGHT, // Aspect ratio
                                                0.1f, 100.0f); // Near and far planes

        // Pass view and projection matrices to shader
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Change background to black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLineWidth(2);
		glPointSize(5);
		// float angle = (GLfloat)glfwGetTime(); // Remove unused angle variable
		// glBindVertexArray(VAO); // Remove this line

		// Get currently selected cube index (last cube in vector)
		// size_t selectedCubeIndex = cubeOffsets.empty() ? 0 : cubeOffsets.size() - 1; // Remove this line

		// Iterate through the dynamic vector
		// for (size_t i = 0; i < cubeOffsets.size(); i++) { // Remove this line
        for (size_t i = 0; i < models.size(); i++) {
			glm::mat4 model = glm::mat4(1);

			// Only apply transformations to the selected (last) cube
			// if (i == selectedCubeIndex) { // Remove this line
            if (i == selectedModelIndex) {
				// Apply transformations in correct order
				// model = glm::translate(model, glm::vec3(translateX, translateY, translateZ)); // Global translation // Remove this line
                // models[i].position += glm::vec3(translateX, translateY, translateZ); // Apply translation to model's position // Remove old translation logic
                // models[i].scale *= scale; // Apply scaling to model's scale // Remove old scaling logic

                // Apply continuous translation
                if (isMovingForward) models[i].position.z -= translationSpeed * deltaTime;
                if (isMovingBackward) models[i].position.z += translationSpeed * deltaTime;
                if (isMovingLeft) models[i].position.x -= translationSpeed * deltaTime;
                if (isMovingRight) models[i].position.x += translationSpeed * deltaTime;
                if (isMovingUp) models[i].position.y += translationSpeed * deltaTime;
                if (isMovingDown) models[i].position.y -= translationSpeed * deltaTime;

                // Apply continuous scaling
                if (isScalingUp) models[i].scale *= (1.0f + (scalingSpeed - 1.0f) * deltaTime);
                if (isScalingDown) models[i].scale *= (1.0f - (scalingSpeed - 1.0f) * deltaTime);

				// Apply rotation around current position
				if (rotateX || rotateY || rotateZ) {
					// glm::vec3 cubeCenter = cubeOffsets[i]; // Get cube's position // Remove this line
					// model = glm::translate(model, cubeCenter); // Move to cube position // Remove this line

					if (rotateX)
						models[i].rotation.x += rotationSpeed * deltaTime; // Apply rotation to model's rotation
					else if (rotateY)
						models[i].rotation.y += rotationSpeed * deltaTime; // Apply rotation to model's rotation
					else if (rotateZ)
						models[i].rotation.z += rotationSpeed * deltaTime; // Apply rotation to model's rotation

					// model = glm::translate(model, -cubeCenter); // Move back // Remove this line
				}
                // Reset temporary transformation values // Remove resetting of temporary variables
                // translateX = translateY = translateZ = 0.0f;
                // scale = 1.0f;
                // rotateX = rotateY = rotateZ = false;
			}

			// Apply cube's base position
			// model = glm::translate(model, cubeOffsets[i]); // Remove this line

            // Apply model's transformations
            model = glm::translate(model, models[i].position);
            model = glm::rotate(model, glm::radians(models[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(models[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(models[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(models[i].scale));


			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			// glDrawArrays(GL_TRIANGLES, 0, 36); // Remove this line
            glBindVertexArray(models[i].VAO); // Bind the model's VAO
            glDrawArrays(GL_TRIANGLES, 0, models[i].numVertices); // Draw the model
            glBindVertexArray(0); // Unbind VAO
		}
		// glBindVertexArray(0); // Remove this line
		glfwSwapBuffers(window);
	}
	// Request OpenGL to deallocate buffers
	// glDeleteVertexArrays(1, &VAO); // Remove this line
    // Delete all model VAOs
    for (const auto& model : models) {
        glDeleteVertexArrays(1, &model.VAO);
    }
	// Terminate GLFW execution, cleaning up allocated resources
	glfwTerminate();
	return 0;
}

// Keyboard callback function - can only have one instance (must be static if
// inside a class) - Called whenever a key is pressed or released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Rotation
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		rotateX = true; rotateY = false; rotateZ = false;
	}
	if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
		rotateX = false; rotateY = true; rotateZ = false;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		rotateX = false; rotateY = false; rotateZ = true;
	}

	// Translation X/Z (WASD)
	if (key == GLFW_KEY_W) isMovingForward = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_S) isMovingBackward = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_A) isMovingLeft = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_D) isMovingRight = (action != GLFW_RELEASE);
	// Translation Y (I/J)
	if (key == GLFW_KEY_I) isMovingUp = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_J) isMovingDown = (action != GLFW_RELEASE);
	// Scale ([ and ])
	if (key == GLFW_KEY_LEFT_BRACKET) isScalingDown = (action != GLFW_RELEASE);
	if (key == GLFW_KEY_RIGHT_BRACKET) isScalingUp = (action != GLFW_RELEASE);

    // Select next model on 'M' press
    // if (key == GLFW_KEY_M && action == GLFW_PRESS) { // Remove this block
    //     selectedModelIndex = (selectedModelIndex + 1) % models.size();
    //     std::cout << "Selected model index: " << selectedModelIndex << std::endl;
    // }

}

// Mouse button callback function
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Get viewport dimensions
        int viewportWidth, viewportHeight;
        glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);

        // Convert mouse coordinates to Normalized Device Coordinates (NDC)
        float x = (2.0f * xpos) / viewportWidth - 1.0f;
        float y = 1.0f - (2.0f * ypos) / viewportHeight;
        float z = 1.0f; // Assuming click is on the far plane for ray direction

        glm::vec3 ray_nds = glm::vec3(x, y, z);

        // Convert NDC to Clip Space
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f); // Use -1.0 for z to get a ray pointing into the scene

        // Convert Clip Space to Eye Space
        glm::vec4 ray_eye = glm::inverse(projectionMatrix) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f); // Set z to -1.0 and w to 0.0 for a direction vector

        // Convert Eye Space to World Space
        glm::vec3 ray_wor = glm::vec3(glm::inverse(viewMatrix) * ray_eye);
        glm::vec3 rayDir = glm::normalize(ray_wor);
        glm::vec3 rayOrigin = glm::vec3(0.0f, 0.0f, 5.0f); // Camera position

        float closestDistance = std::numeric_limits<float>::max();
        int intersectedModelIndex = -1;

        for (size_t i = 0; i < models.size(); ++i)
        {
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), models[i].position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(models[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            modelMatrix = glm::scale(modelMatrix, glm::vec3(models[i].scale));

            glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

            glm::vec3 localRayOrigin = glm::vec3(inverseModelMatrix * glm::vec4(rayOrigin, 1.0f));
            glm::vec3 localRayDir = glm::vec3(inverseModelMatrix * glm::vec4(rayDir, 0.0f));

            // Iterate through triangles (assuming vertices are in groups of 3 for triangles)
            for (size_t j = 0; j < models[i].vertices.size(); j += 3)
            {
                glm::vec3 v0 = models[i].vertices[j];
                glm::vec3 v1 = models[i].vertices[j + 1];
                glm::vec3 v2 = models[i].vertices[j + 2];

                float distance;
                if (intersectTriangle(localRayOrigin, localRayDir, v0, v1, v2, distance))
                {
                    if (distance < closestDistance)
                    {
                        closestDistance = distance;
                        intersectedModelIndex = i;
                    }
                }
            }
        }

        if (intersectedModelIndex != -1)
        {
            selectedModelIndex = intersectedModelIndex;
            std::cout << "Clicked on model: " << selectedModelIndex << std::endl;
        }
    }
}

// This function is quite hardcoded - objective is to compile and build a simple and unique shader program in this code example
// The vertex and fragment shader source code is in the vertexShaderSource and fragmentShader source arrays at the beginning of this file
// The function returns the shader program identifier
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Check for compilation errors (display via log in the terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Check for compilation errors (display via log in the terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Link shaders and create shader program identifier
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// This function is quite hardcoded - objective is to create the buffers that store the
// triangle geometry
// Only coordinate attribute in vertices
// 1 VBO with coordinates, VAO with only 1 attribute pointer
// The function returns the VAO identifier
int setupGeometry()
{
	// Here we set the x, y, and z coordinates of the triangle and store them sequentially,
	// already aiming to send them to the VBO (Vertex Buffer Objects)
	// Each vertex attribute (coordinate, colors, texture coordinates, normal, etc.)
	// Can be stored in a single VBO or in separate VBOs
	GLfloat vertices[] = {
		// Face +X (red)
		 0.5, -0.5, -0.5, 1,0,0,   0.5,  0.5, -0.5, 1,0,0,   0.5,  0.5,  0.5, 1,0,0,
		 0.5, -0.5, -0.5, 1,0,0,   0.5,  0.5,  0.5, 1,0,0,   0.5, -0.5,  0.5, 1,0,0,
		// Face -X (green)
		-0.5, -0.5,  0.5, 0,1,0,  -0.5,  0.5,  0.5, 0,1,0,  -0.5,  0.5, -0.5, 0,1,0,
		-0.5, -0.5,  0.5, 0,1,0,  -0.5,  0.5, -0.5, 0,1,0,  -0.5, -0.5, -0.5, 0,1,0,
		// Face +Y (blue)
		-0.5,  0.5, -0.5, 0,0,1,   0.5,  0.5, -0.5, 0,0,1,   0.5,  0.5,  0.5, 0,0,1,
		-0.5,  0.5, -0.5, 0,0,1,   0.5,  0.5,  0.5, 0,0,1,  -0.5,  0.5,  0.5, 0,0,1,
		// Face -Y (yellow)
		-0.5, -0.5,  0.5, 1,1,0,   0.5, -0.5,  0.5, 1,1,0,   0.5, -0.5, -0.5, 1,1,0,
		-0.5, -0.5,  0.5, 1,1,0,   0.5, -0.5, -0.5, 1,1,0,  -0.5, -0.5, -0.5, 1,1,0,
		// Face +Z (magenta)
		-0.5, -0.5,  0.5, 1,0,1,  -0.5,  0.5,  0.5, 1,0,1,   0.5,  0.5,  0.5, 1,0,1,
		-0.5, -0.5,  0.5, 1,0,1,   0.5,  0.5,  0.5, 1,0,1,   0.5, -0.5,  0.5, 1,0,1,
		// Face -Z (cyan)
		-0.5,  0.5, -0.5, 0,1,1,  -0.5, -0.5, -0.5, 0,1,1,   0.5, -0.5, -0.5, 0,1,1,
		-0.5,  0.5, -0.5, 0,1,1,   0.5, -0.5, -0.5, 0,1,1,   0.5,  0.5, -0.5, 0,1,1,
	};

	GLuint VBO, VAO;

	// Generate VBO identifier
	glGenBuffers(1, &VBO);

	// Bind the buffer as an array buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Send the float array data to the OpenGL buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Generate VAO (Vertex Array Object) identifier
	glGenVertexArrays(1, &VAO);

	// Bind the VAO first, then connect and set the vertex buffer(s)
	// and attribute pointers
	glBindVertexArray(VAO);
	
	// For each vertex attribute, create an "AttribPointer" (pointer to the attribute), indicating:
	// Location in the shader * (attribute locations must match the layout specified in the vertex shader)
	// Number of values the attribute has (e.g., 3 xyz coordinates)
	// Data type
	// Whether it is normalized (between zero and one)
	// Size in bytes
	// Offset from byte zero
	
	// Position attribute (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Color attribute (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);


	// Note that this is allowed, the call to glVertexAttribPointer registered the VBO as the currently
	// bound vertex buffer object - so we can safely unbind afterwards
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Unbind the VAO (it's good practice to unbind any buffer or array to avoid nasty bugs)
	glBindVertexArray(0);

	return VAO;
}

