#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>
#include <render/texture.h>
#include <models/box.h>

#include <vector>
#include <iostream>
#include <math.h>
#include <models/sphere.h>

#define _USE_MATH_DEFINES

static bool useSphereScene = false; // false => boxes, true => spheres

static Sphere sphere; // The new sphere object

static GLFWwindow *window;
static int windowWidth = 1024; 
static int windowHeight = 768;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 originalEyeCenter(0, 0, 100);

static glm::vec3 eyeCenter = originalEyeCenter;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

static glm::float32 FoV = 45;
static glm::float32 zNear = 0.1f; 
static glm::float32 zFar = 1000.0f;

// View control 
static float viewAzimuth = M_PI / 2;
static float viewPolar = M_PI / 2;
static float viewDistance = 100.0f;
static bool rotating = false;

// Scene control 
static int numBoxes = 1;				// Debug: set numBoxes to 1.
std::vector<glm::mat4> boxTransforms;	// We represent the scene by a single box and a number of transforms for drawing the box at different locations.

// Anaglyph control 
static float ipd = 2.0f;				// Distance between left/right eye.
// After you implement the anaglyph, adjust the IPD value to control the red/cyan offsets and depth perception. 

enum AnaglyphMode {
	None,
	ToeIn, 
	Asymmetric, 
	AnaglyphModeCount,
};

static std::string strAnaglyphMode[] = {
	"None", 
	"Toe-in", 
	"Asymmetric view frustum", 
	"Invalid",
};

static AnaglyphMode anaglyphMode = AnaglyphMode::None;

// Helper functions 

static void nextAnaglyphMode() {
	anaglyphMode = (AnaglyphMode)(((int)anaglyphMode + 1) % (int)AnaglyphModeCount);
}

static int randomInt() {
	return rand();
}

static float randomFloat() {
	float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	return r;
}

static glm::vec3 randomVec3() {
	return glm::vec3(randomFloat(), randomFloat(), randomFloat());
}

static void generateScene() {
	boxTransforms.clear();
	if (numBoxes == 1) {
		// Use this for debugging
		glm::mat4 modelMatrix = glm::mat4();
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(16, 16, 16));
		boxTransforms.push_back(modelMatrix);
	} else {
		// Generate boxes based on random position, rotation, and scale. 
		// Store their transforms.
		for (int i = 0; i < numBoxes; ++i) {
			glm::vec3 position = 100.0f * (randomVec3() - 0.5f);
			float s = (1 + (randomInt() % 4)) * 1.0f;
			glm::vec3 scale(s, s, s);
			float angle = randomFloat() * M_PI * 2;
			glm::vec3 axis = glm::normalize(randomVec3() - 0.5f);

			glm::mat4 modelMatrix = glm::mat4();
			modelMatrix = glm::translate(modelMatrix, position);
			modelMatrix = glm::rotate(modelMatrix, angle, axis);
			modelMatrix = glm::scale(modelMatrix, scale);
			boxTransforms.push_back(modelMatrix);
		}
	}
}

// Debugging functions 

static void printAnaglyphMode() {
	std::cout << "Anaglyph mode: " << strAnaglyphMode[(int)anaglyphMode] << std::endl;
}

static void printVec3(glm::vec3 v) {
	std::cout << v.x << " " << v.y << " " << v.z << std::endl;
}

static void printMat4(glm::mat4 m) {
	// Column major
	std::cout << m[0][0] << " " << m[1][0] << " " << m[2][0] << " " << m[3][0] << std::endl;
	std::cout << m[0][1] << " " << m[1][1] << " " << m[2][1] << " " << m[3][1] << std::endl;
	std::cout << m[0][2] << " " << m[1][2] << " " << m[2][2] << " " << m[3][2] << std::endl;
	std::cout << m[0][3] << " " << m[1][3] << " " << m[2][3] << " " << m[3][3] << std::endl;
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(windowWidth, windowHeight, "Anaglyph Rendering", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Ensure we can capture mouse cursor movement 
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	srand(2024);

	// Background
	glClearColor(163 / 255.0f, 227 / 255.0f, 255 / 255.0f, 1.0f);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Create a box
	Box box;
	box.initialize();

	sphere.initialize();

	// Create the scene with a set of boxes represented by their transforms
	generateScene();

	// Set a perspective camera 
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);

	printAnaglyphMode();

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO: Render anaglyph 
		// --------------------------------------------------------------------

		if (anaglyphMode == None)
		{
			// Clear the screen
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Set camera view matrix
			glm::mat4 viewMatrix = glm::lookAt(eyeCenter, lookat, up);
			glm::mat4 vp = projectionMatrix * viewMatrix;

			// If we’re in sphere scene, render spheres. Otherwise, render boxes.
			if (!useSphereScene)
			{
				for (int i = 0; i < numBoxes; ++i)
				{
					box.render(vp, boxTransforms[i]);
				}
			}
			else
			{
				for (int i = 0; i < numBoxes; ++i)
				{
					sphere.render(vp, boxTransforms[i]);
				}
			}
		}
		else
		{

			glm::mat4 vpLeft;
			glm::mat4 vpRight;

			if (anaglyphMode == ToeIn) {

				// TODO: Implement the toe-in projection here
				// 1) Compute the camera’s right direction
				glm::vec3 rightDir = glm::normalize(glm::cross(lookat - eyeCenter, up));

				// 2) Shift each eye left/right by half the IPD
				glm::vec3 leftEyePos = eyeCenter - 0.5f * ipd * rightDir;
				glm::vec3 rightEyePos = eyeCenter + 0.5f * ipd * rightDir;

				// 3) “Toe in”: each eye rotates to converge on the same lookat point
				glm::mat4 viewLeft = glm::lookAt(leftEyePos, lookat, up);
				glm::mat4 viewRight = glm::lookAt(rightEyePos, lookat, up);

				// 4) Use the same perspective projection for both eyes
				vpLeft = projectionMatrix * viewLeft;
				vpRight = projectionMatrix * viewRight;

				// ------------------------------------------------------------


			} else if (anaglyphMode == Asymmetric) {	

				// TODO: Implement the asymmetric view frustum here
				float aspect = float(windowWidth) / float(windowHeight);
				float top = zNear * tan(glm::radians(FoV / 2.0f));
				float rightVal = top * aspect;

				// Distance from camera to the plane you’re focusing on.
				// Here, we use “viewDistance” since the camera is ~100 units from the origin.
				float frustumShift = 0.5f * ipd * (zNear / viewDistance);

				// Compute shared directions
				glm::vec3 forwardDir = glm::normalize(lookat - eyeCenter);
				glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, up));

				// Left eye position (shift by -ipd/2)
				glm::vec3 leftEyePos = eyeCenter - 0.5f * ipd * rightDir;
				// Right eye position (shift by +ipd/2)
				glm::vec3 rightEyePos = eyeCenter + 0.5f * ipd * rightDir;

				// For the left eye, frustum is shifted to the right by “frustumShift”
				float left_left = -rightVal + frustumShift;
				float left_right = rightVal + frustumShift;
				glm::mat4 projLeft = glm::frustum(left_left, left_right, -top, top, zNear, zFar);
				glm::mat4 viewLeft = glm::lookAt(leftEyePos, leftEyePos + forwardDir, up);
				vpLeft = projLeft * viewLeft;

				// For the right eye, frustum is shifted left by “frustumShift”
				float right_left = -rightVal - frustumShift;
				float right_right = rightVal - frustumShift;
				glm::mat4 projRight = glm::frustum(right_left, right_right, -top, top, zNear, zFar);
				glm::mat4 viewRight = glm::lookAt(rightEyePos, rightEyePos + forwardDir, up);
				vpRight = projRight * viewRight;

				// ------------------------------------------------------------

			}

			// TODO: Implement two-pass rendering to draw the anaglyph
			// FIRST PASS: Render the Left Eye in Red only
			glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE); // R only
			glClear(GL_DEPTH_BUFFER_BIT);					   // Clear depth but keep color
			if (!useSphereScene)
			{
				// BOX RENDER
				for (int i = 0; i < numBoxes; ++i)
				{
					box.render(vpLeft, boxTransforms[i]);
				}
			}
			else
			{
				// SPHERE RENDER
				// Note: you can re-use boxTransforms for the sphere's positions/scales
				// or define a separate sphereTransforms if you prefer.
				for (int i = 0; i < numBoxes; ++i)
				{
					sphere.render(vpLeft, boxTransforms[i]);
				}
			}

			// SECOND PASS: Render the Right Eye in Cyan (G+B) only
			// SECOND PASS: Render the Right Eye in Cyan (G+B) only
			glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); // G+B
			glClear(GL_DEPTH_BUFFER_BIT);					  // Clear depth again
			if (!useSphereScene)
			{
				// BOX RENDER
				for (int i = 0; i < numBoxes; ++i)
				{
					box.render(vpRight, boxTransforms[i]);
				}
			}
			else
			{
				// SPHERE RENDER
				for (int i = 0; i < numBoxes; ++i)
				{
					sphere.render(vpRight, boxTransforms[i]);
				}
			}

			// Finally, restore normal color masking
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			// ----------------------------------------------------------------
		}

		// --------------------------------------------------------------------


		// Animation
		static double lastTime = glfwGetTime();
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;
		if (rotating) {
			viewAzimuth += 1.0f * deltaTime;
			eyeCenter.x = viewDistance * cos(viewAzimuth);
			eyeCenter.z = viewDistance * sin(viewAzimuth);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	sphere.cleanup();
	box.cleanup();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		std::cout << "Space key is pressed." << std::endl;
		rotating = !rotating;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		std::cout << "Reset." << std::endl;
		rotating = false;
		eyeCenter = originalEyeCenter;
		viewAzimuth = M_PI / 2;
		viewPolar = M_PI / 2;
	}

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar -= 0.1f;
		eyeCenter.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar += 0.1f;
		eyeCenter.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth -= 0.1f;
		eyeCenter.x = viewDistance * cos(viewAzimuth);
		eyeCenter.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth += 0.1f;
		eyeCenter.x = viewDistance * cos(viewAzimuth);
		eyeCenter.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        nextAnaglyphMode(); 
		printAnaglyphMode();
	}

	// Adjust the IPD value to match your actual viewing distance
	// Special case: IPD == 0 means no 3D effect.

	if (key == GLFW_KEY_COMMA) {
		ipd -= 0.1f;
		ipd = std::max(ipd, 0.0f);
		std::cout << "IPD: " << ipd << std::endl;
	}

	if (key == GLFW_KEY_PERIOD) {
		ipd += 0.1f;
		std::cout << "IPD: " << ipd << std::endl;
	}

	if (key == GLFW_KEY_1) {
		numBoxes = 1;
		generateScene();
	}

	if (key == GLFW_KEY_0) {
		numBoxes = 100;
		generateScene();
	}

	// Press '2' to toggle sphere mode
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		useSphereScene = !useSphereScene;
		if (useSphereScene)
		{
			std::cout << "Switched to Sphere scene.\n";
			// If you want to regenerate a random sphere scene
			// or re-use the same transforms, do it here:
			generateScene();
			// Or create a separate generateSphereScene() if you want different logic
		}
		else
		{
			std::cout << "Switched to Box scene.\n";
			generateScene();
		}
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	// Optionally, you can implement your own mouse support.
}
