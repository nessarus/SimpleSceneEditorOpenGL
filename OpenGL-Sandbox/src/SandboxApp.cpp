#include "GLCore.h"
#include "SandboxLayer.h"
#include <filesystem>

int windowHeight = 640, windowWidth = 960;
char dataDir[256];  // Stores the path to the models-textures folder.
char* programName = nullptr;

using namespace GLCore;

class Sandbox : public Application
{
public:
	Sandbox(const std::string& _programName, uint32_t width, uint32_t height)
		: Application(_programName, width, height)
	{
		PushLayer(new SandboxLayer(width, height));
	}
};

//----------------------------------------------------------------------------

char dirDefault1[] = "assets/models-textures";

void fileErr(char* fileName)
{
	printf("Error reading file: %s\n", fileName);
	printf("When not in the CSSE labs, you will need to include the directory containing\n");
	printf("the models on the command line, or put it in the same folder as the exectutable.");
	exit(1);
}

//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	// Get the program name, excluding the directory, for the window title
	programName = argv[0];
	for (char* cpointer = argv[0]; *cpointer != 0; cpointer++)
		if (*cpointer == '/' || *cpointer == '\\') programName = cpointer + 1;

	// Set the models-textures directory, via the first argument or some handy defaults.
	if (argc > 1)
		strcpy_s(dataDir, sizeof(dataDir), argv[1]);
	else if (std::filesystem::exists(dirDefault1)) 
		strcpy_s(dataDir, sizeof(dataDir), dirDefault1);
	else 
		fileErr(dirDefault1);

	std::unique_ptr<Sandbox> app = std::make_unique<Sandbox>(programName, windowWidth, windowHeight);
	app->Run();
}