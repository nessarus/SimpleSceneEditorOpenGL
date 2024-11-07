#pragma once
// Graphics and Animation Tool Interface and Data Reader (gnatiread.h)

// You shouldn't need to modify the code in this file, but feel free to.
// If you do, it would be good to mark your changes with comments.

#include "GLCore/Core/MouseButtonCodes.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Define a helpful macro for handling offsets into buffer objects
#define BUFFER_OFFSET(offset)   ((const void*)(offset))

extern int windowHeight, windowWidth;
extern char dataDir[256];  // Stores the path to the models-textures folder.
const int numTextures = 31;
const int numMeshes = 56;


// ------Functions to fail with an error mesage then a string or int------ 

void fail(const char* msg1, char* msg2) {
    fprintf(stderr, "%s %s\n", msg1, msg2);
    exit(1);
}

void failInt(const char* msg1, int i) {
    fprintf(stderr, "%s %d\n", msg1, i);
    exit(1);
}


// -----Texture data reading--------------------------------------------

// A type for a 2D texture, with height and width in pixels
typedef struct {
    int height;
    int width;
    uint8_t* rgbData;   // Array of bytes with the colour data for the texture
} texture;


// Load a texture
texture* loadTexture(char* fileName) {
    texture* t = (texture*)malloc(sizeof(texture));

    if (t != nullptr)
    {
        int x, y, n;
        t->rgbData = stbi_load(fileName, &x, &y, &n, 0);
        if (t->rgbData == NULL) fail("Error loading image: ", fileName);

        t->height = y;
        t->width = x;

        printf("\nLoaded a %d by %d texture\n\n", t->height, t->width);
    }
    return t;
}

// Load the texture with number num from the models-textures directory
texture* loadTextureNum(int num) {
    if (num < 0 || num >= numTextures)
        failInt("Error in loading texture - wrong texture number:", num);

    char fileName[220];
    sprintf_s(fileName, sizeof(fileName), "%s/texture%d.bmp", dataDir, num);
    return loadTexture(fileName);
}

//----------------------------------------------------------------------------

// Initialise the Open Asset Importer toolkit
void aiInit()
{
    struct aiLogStream stream;

    // get a handle to the predefined STDOUT log stream and attach
    // it to the logging system. It remains active for all further
    // calls to aiImportFile(Ex) and aiApplyPostProcessing.
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
    aiAttachLogStream(&stream);

    // ... same procedure, but this stream now writes the
    // log messages to assimp_log.txt
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
    aiAttachLogStream(&stream);
}


// Load a mesh by number from the models-textures directory via the Open Asset Importer
aiMesh* loadMesh(int meshNumber) {
    char filename[256];
    sprintf_s(filename, sizeof(filename), "%s/model%d.x", dataDir, meshNumber);
    const aiScene* scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_Quality
        | aiProcess_ConvertToLeftHanded);
    return scene->mMeshes[0];
}


// -------------- Strings for the texture and mesh menus ---------------------------------

char textureMenuEntries[numTextures][128] = {
    "1 Plain", "2 Rust", "3 Concrete", "4 Carpet", "5 Beach Sand",
    "6 Rocky", "7 Brick", "8 Water", "9 Paper", "10 Marble",
    "11 Wood", "12 Scales", "13 Fur", "14 Denim", "15 Hessian",
    "16 Orange Peel", "17 Ice Crystals", "18 Grass", "19 Corrugated Iron", "20 Styrofoam",
    "21 Bubble Wrap", "22 Leather", "23 Camouflage", "24 Asphalt", "25 Scratched Ice",
    "26 Rattan", "27 Snow", "28 Dry Mud", "29 Old Concrete", "30 Leopard Skin"
};

char objectMenuEntries[numMeshes][128] = {
    "1 Thin Dinosaur","2 Big Dog","3 Saddle Dinosaur", "4 Dragon", "5 Cleopatra",
    "6 Bone I", "7 Bone II", "8 Rabbit", "9 Long Dragon", "10 Buddha",
    "11 Sitting Rabbit", "12 Frog", "13 Cow", "14 Monster", "15 Sea Horse",
    "16 Head", "17 Pelican", "18 Horse", "19 Kneeling Angel", "20 Porsche I",
    "21 Truck", "22 Statue of Liberty", "23 Sitting Angel", "24 Metal Part", "25 Car",
    "26 Apatosaurus", "27 Airliner", "28 Motorbike", "29 Dolphin", "30 Spaceman",
    "31 Winnie the Pooh", "32 Shark", "33 Crocodile", "34 Toddler", "35 Fat Dinosaur",
    "36 Chihuahua", "37 Sabre-toothed Tiger", "38 Lioness", "39 Fish", "40 Horse (head down)",
    "41 Horse (head up)", "42 Skull", "43 Fighter Jet I", "44 Toad", "45 Convertible",
    "46 Porsche II", "47 Hare", "48 Vintage Car", "49 Fighter Jet II", "50 Gargoyle",
    "51 Chef", "52 Parasaurolophus", "53 Rooster", "54 T-rex", "55 Sphere"
};

//-----Code for using the mouse to adjust floats - you shouldn't need to modify this code.
// Calling setTool(vX, vY, vMat, wX, wY, wMat) below makes the left button adjust *vX and *vY
// as the mouse moves in the X and Y directions, via the transformation vMat which can be used
// for scaling and rotation. Similarly the middle button adjusts *wX and *wY via wMat.
// Any of vX, vY, wX, wY may be NULL in which case nothing is adjusted for that component.


static glm::vec2 prevPos;
static glm::mat2 leftTrans, middTrans;
static int currButton = -1;

static void doNothingCallback(glm::vec2 xy) { return; }

static void(*leftCallback)(glm::vec2) = &doNothingCallback;
static void(*middCallback)(glm::vec2) = &doNothingCallback;

static float mouseX = 0, mouseY = 0;         // Updated in the mouse-passive-motion function.

static glm::vec2 currMouseXYscreen(float x, float y) {
    return glm::vec2(x / windowWidth, (windowHeight - y) / windowHeight);
}

static void doToolUpdateXY(float x, float y) {
    if (currButton == HZ_MOUSE_BUTTON_LEFT || currButton == HZ_MOUSE_BUTTON_MIDDLE) {
        glm::vec2 currPos = glm::vec2(currMouseXYscreen(x, y));
        if (currButton == HZ_MOUSE_BUTTON_LEFT)
            leftCallback(leftTrans * (currPos - prevPos));
        else
            middCallback(middTrans * (currPos - prevPos));

        prevPos = currPos;
    }
}

static glm::mat2 rotZ(float rotSidewaysDeg) {
    glm::mat4 rot4 = glm::rotate(glm::mat4(1.0f), glm::radians(rotSidewaysDeg), glm::vec3(0, 0, 1));
    return glm::mat2(rot4[0][0], rot4[0][1], rot4[1][0], rot4[1][1]);
}

//static vec2 currXY(float rotSidewaysDeg) { return rotZ(rotSidewaysDeg) * vec2(currRawX(), currRawY()); }
static glm::vec2 currMouseXYworld(float rotSidewaysDeg) { return rotZ(rotSidewaysDeg) * currMouseXYscreen(mouseX, mouseY); }

// See the comment about 40 lines above
static void setToolCallbacks(void (*newLeftCallback)(glm::vec2 transformedMovement), glm::mat2 leftT,
    void (*newMiddCallback)(glm::vec2 transformedMovement), glm::mat2 middT) {

    leftCallback = newLeftCallback;
    leftTrans = leftT;
    middCallback = newMiddCallback;
    middTrans = middT;

    currButton = -1;  // No current button to start with

    // std::cout << leftXYold << " " << middXYold << std::endl; // For debugging
}

glm::vec2 clickPrev;

static void activateTool(int button) {
    currButton = button;
    clickPrev = currMouseXYscreen(mouseX, mouseY);

    //std::cout << mouseX << " " << mouseY << std::endl;  // For debugging
}

static void deactivateTool() {
    currButton = -1;
}

//-------------------------------------------------------------
