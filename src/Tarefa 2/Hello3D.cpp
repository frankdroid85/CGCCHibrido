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
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
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

bool rotateX=false, rotateY=false, rotateZ=false;
float translateX = 0.0f, translateY = 0.0f, translateZ = 0.0f;
float scale = 1.0f;

// Use a vector for dynamic cube offsets
std::vector<glm::vec3> cubeOffsets;

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
	GLuint VAO = setupGeometry();

    // Initial cube offset - start with just one cube at the origin
    cubeOffsets.push_back(glm::vec3(0.0f, 0.0f, 0.0f));


	glUseProgram(shaderID);

	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	glEnable(GL_DEPTH_TEST);

	// Application loop - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLineWidth(2);
		glPointSize(5);
		float angle = (GLfloat)glfwGetTime();
		glBindVertexArray(VAO);
		
		// Get currently selected cube index (last cube in vector)
		size_t selectedCubeIndex = cubeOffsets.empty() ? 0 : cubeOffsets.size() - 1;

		// Iterate through the dynamic vector
		for (size_t i = 0; i < cubeOffsets.size(); i++) {
			glm::mat4 model = glm::mat4(1);
			
			// Only apply transformations to the selected (last) cube
			if (i == selectedCubeIndex) {
				// Apply transformations in correct order
				model = glm::translate(model, glm::vec3(translateX, translateY, translateZ)); // Global translation
				
				// Apply rotation around current position
				if (rotateX || rotateY || rotateZ) {
					glm::vec3 cubeCenter = cubeOffsets[i]; // Get cube's position
					model = glm::translate(model, cubeCenter); // Move to cube position
					
					if (rotateX)
						model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
					else if (rotateY)
						model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
					else if (rotateZ)
						model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
					
					model = glm::translate(model, -cubeCenter); // Move back
				}
			}
			
			// Apply cube's base position
			model = glm::translate(model, cubeOffsets[i]);
			
			// Scale (affects all cubes)
			model = glm::scale(model, glm::vec3(scale));
			
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);
		glfwSwapBuffers(window);
	}
	// Request OpenGL to deallocate buffers
	glDeleteVertexArrays(1, &VAO);
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
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateZ -= 0.1f;
	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateZ += 0.1f;
	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateX -= 0.1f;
	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateX += 0.1f;
	// Translation Y (I/J)
	if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateY += 0.1f;
	if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateY -= 0.1f;
	// Scale ([ and ])
	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scale *= 0.9f;
	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scale *= 1.1f;    // Add new cube on 'N' press with random offset
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        // Generate random position for the new cube
        glm::vec3 newPos(dis(gen), dis(gen), dis(gen));
        // Ensure some minimum distance from other cubes to prevent overlap
        bool validPosition = true;
        for (const auto& offset : cubeOffsets) {
            if (glm::length(newPos - offset) < 1.0f) { // Check if too close to existing cube
                validPosition = false;
                break;
            }
        }
        if (validPosition || cubeOffsets.empty()) {
            cubeOffsets.push_back(newPos);
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

