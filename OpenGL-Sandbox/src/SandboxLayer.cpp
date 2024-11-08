#include "SandboxLayer.h"

#include "GLCore/Core/KeyCodes.h"
// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data
// Reader" code.  This file contains parts of the code that you shouldn't need
// to modify (but, you can).
#include "gnatidread.h"
#include <imgui.h>

// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, modelViewU; // IDs for uniform variables (from glGetUniformLocation)

static float viewDist = 1.5; // Distance from the camera to the centre of the scene
static float camRotSidewaysDeg = 0; // rotates the camera sideways around the centre
static float camRotUpAndOverDeg = 20; // rotates the camera up and over the centre.

glm::mat4 projection; // Projection matrix - set in the reshape function
glm::mat4 view; // View matrix - set in the display function.

// These are used to set the window title
extern char* programName; // Set in main 
char title[256];
char lab[] = "Project1";
int numDisplayCalls = 0; // Used to calculate the number of frames per second
float displayTimer_msecs = 0;

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                           (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    glm::vec4 loc;
    float scale;
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    glm::vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;    // How many objects are currenly in the scene.
int currObject = -1; // The current object
int toolObj = -1;    // The object currently being modified

//----------------------------------------------------------------------------
//
// Loads a texture by number, and binds it for later use.    
void loadTextureIfNotAlreadyLoaded(int i)
{
    if (textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i);
    glActiveTexture(GL_TEXTURE0);

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
        0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0); // Back to default texture
}

//------Mesh loading----------------------------------------------------------
//
// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber)
{
    if (meshNumber>=numMeshes || meshNumber < 0) {
        printf("Error - no such model number");
        exit(1);
    }

    if (meshes[meshNumber] != NULL)
        return; // Already loaded

    aiMesh* mesh = loadMesh(meshNumber);
    meshes[meshNumber] = mesh;

    glBindVertexArray( vaoIDs[meshNumber] );

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers( 1, buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices,
                  NULL, GL_STATIC_DRAW );

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.    
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);

    // Load the element index data
    std::vector<GLuint> elements(mesh->mNumFaces * 3);
    for (GLuint i=0; i < mesh->mNumFaces; i++) {
        elements[i*3] = mesh->mFaces[i].mIndices[0];
        elements[i*3+1] = mesh->mFaces[i].mIndices[1];
        elements[i*3+2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements.data(), GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0                 
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( vPosition );

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) );
    glEnableVertexAttribArray( vNormal );
}

//----------------------------------------------------------------------------

static void mouseClickOrScroll(int button, bool pressed, int mods, float x, float y)
{
    if (button == HZ_MOUSE_BUTTON_LEFT && pressed) {
        if (mods != HZ_MOD_SHIFT) activateTool(button);
        else activateTool(HZ_MOUSE_BUTTON_LEFT);
    }
    else if (button == HZ_MOUSE_BUTTON_LEFT && !pressed) deactivateTool();
    else if (button == HZ_MOUSE_BUTTON_MIDDLE && pressed) { activateTool(button); }
    else if (button == HZ_MOUSE_BUTTON_MIDDLE && !pressed) deactivateTool();

    else if (button == 3) { // scroll up
        viewDist = (viewDist < 0.0f ? viewDist : viewDist * 0.8f) - 0.05f;
    }
    else if (button == 4) { // scroll down
        viewDist = (viewDist < 0.0f ? viewDist : viewDist * 1.25f) + 0.05f;
    }
}

//----------------------------------------------------------------------------

static void mousePassiveMotion(float x, float y)
{
    mouseX = x;
    mouseY = y;
}

//----------------------------------------------------------------------------

glm::mat2 camRotZ()
{
    return rotZ(-camRotSidewaysDeg) * glm::mat2(10.0f, 0.0f, 0.0f, -10.0f);
}

//------callback functions for doRotate below and later-----------------------

static void adjustCamrotsideViewdist(glm::vec2 cv)
{
    camRotSidewaysDeg += cv[0]; viewDist += cv[1];
}

static void adjustcamSideUp(glm::vec2 su)
{
    camRotSidewaysDeg+=su[0]; camRotUpAndOverDeg+=su[1];
}

static void adjustLocXZ(glm::vec2 xz)
{
    sceneObjs[toolObj].loc[0]+=xz[0]; sceneObjs[toolObj].loc[2]+=xz[1];
}

static void adjustScaleY(glm::vec2 sy)
{
    sceneObjs[toolObj].scale+=sy[0]; sceneObjs[toolObj].loc[1]+=sy[1];
}


//----------------------------------------------------------------------------
//------Set the mouse buttons to rotate the camera----------------------------
//------around the centre of the scene.---------------------------------------
//----------------------------------------------------------------------------

static void doRotate()
{
    setToolCallbacks(adjustCamrotsideViewdist, glm::mat2(400, 0, 0, -2),
        adjustcamSideUp, glm::mat2(400, 0, 0, -90));
}

//------Add an object to the scene--------------------------------------------

static void addObject(int id)
{
    glm::vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
    sceneObjs[nObjects].loc[0] = currPos[0];
    sceneObjs[nObjects].loc[1] = 0.0f;
    sceneObjs[nObjects].loc[2] = currPos[1];
    sceneObjs[nObjects].loc[3] = 1.0f;

    if (id != 0 && id != 55)
        sceneObjs[nObjects].scale = 0.005f;

    sceneObjs[nObjects].rgb[0] = 0.7f; sceneObjs[nObjects].rgb[1] = 0.7f;
    sceneObjs[nObjects].rgb[2] = 0.7f; sceneObjs[nObjects].brightness = 1.0f;

    sceneObjs[nObjects].diffuse = 1.0f; sceneObjs[nObjects].specular = 0.5f;
    sceneObjs[nObjects].ambient = 0.7f; sceneObjs[nObjects].shine = 10.0f;

    sceneObjs[nObjects].angles[0] = 0.0f; sceneObjs[nObjects].angles[1] = 180.0f;
    sceneObjs[nObjects].angles[2] = 0.0f;

    sceneObjs[nObjects].meshId = id;
    sceneObjs[nObjects].texId = rand() % numTextures;
    sceneObjs[nObjects].texScale = 2.0f;

    toolObj = currObject = nObjects++;
    setToolCallbacks(adjustLocXZ, camRotZ(),
        adjustScaleY, glm::mat2(0.05f, 0, 0, 10.0f));
}

void init(void)
{
    // Init here
    srand(static_cast<uint32_t>(time(nullptr))); /* initialize random seed - so the starting scene varies */
    aiInit();

    glGenVertexArrays(numMeshes, vaoIDs);   // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); // Allocate texture objects

    // Initialize the vertex position attribute from the vertex shader        
    vPosition = glGetAttribLocation(shaderProgram, "vPosition");
    vNormal = glGetAttribLocation(shaderProgram, "vNormal");;
    
    // Likewise, initialize the vertex texture coordinates attribute.    
    vTexCoord = glGetAttribLocation(shaderProgram, "vTexCoord");

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    sceneObjs[0].scale = 10.0f;
    sceneObjs[0].angles[0] = 90.0f; // Rotate it.
    sceneObjs[0].angles[0] = 0.0f; // Rotate it.
    sceneObjs[0].texScale = 5.0f; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = glm::vec4(2.0f, 1.0f, 1.0f, 1.0f);
    sceneObjs[1].scale = 0.1f;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.2f; // The light's brightness is 5 times this (below).

    addObject(rand() % numMeshes); // A test mesh

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable(GL_DEPTH_TEST);
    doRotate(); // Start in camera rotate mode.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); /* black background */
}

//----------------------------------------------------------------------------
void drawMesh(SceneObject sceneObj)
{

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb
    // colour of the surface but there could be separate types for, e.g.,
    // specularity and normals.
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 );

    // Set the texture scale for the shaders
    glUniform1f( glGetUniformLocation( shaderProgram, "texScale"), sceneObj.texScale );

    // Set the projection matrix for the shaders
    glUniformMatrix4fv(projectionU, 1, GL_FALSE, glm::value_ptr(projection));

    // Set the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(sceneObj.loc)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(sceneObj.scale));


    // Set the model-view matrix for the shaders
    glUniformMatrix4fv(modelViewU, 1, GL_FALSE, glm::value_ptr(view * model));

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    glBindVertexArray(vaoIDs[sceneObj.meshId]);

    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3,
        GL_UNSIGNED_INT, NULL);
}

//----------------------------------------------------------------------------

void display( void )
{
    numDisplayCalls++;

    // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the view matrix. To start with this just moves the camera
    // backwards.  You'll need to add appropriate rotations.

    view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -viewDist));

    SceneObject lightObj1 = sceneObjs[1];
    glm::vec4 lightPosition = view * lightObj1.loc;

    glUniform4fv(glGetUniformLocation(shaderProgram, "LightPosition"), 
        1, glm::value_ptr(lightPosition));

    for (int i = 0; i < nObjects; i++) {
        SceneObject so = sceneObjs[i];

        glm::vec3 rgb = so.rgb * lightObj1.rgb * so.brightness * lightObj1.brightness * 2.0f;
        glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, glm::value_ptr(so.ambient * rgb));
        glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, glm::value_ptr(so.diffuse * rgb));
        glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, glm::value_ptr(so.specular * rgb));
        glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), so.shine);

        drawMesh(sceneObjs[i]);
    }
}


//----------------------------------------------------------------------------
//------Menus-----------------------------------------------------------------
//----------------------------------------------------------------------------

static void objectMenu(int id)
{
    deactivateTool();
    addObject(id);
}

static void texMenu(int id)
{
    deactivateTool();
    if (currObject>=0) {
        sceneObjs[currObject].texId = id;
    }
}

static void groundMenu(int id)
{
    deactivateTool();
    sceneObjs[0].texId = id;
}

static void adjustBrightnessY(glm::vec2 by)
{
    sceneObjs[toolObj].brightness += by[0];
    sceneObjs[toolObj].loc[1] += by[1];
}

static void adjustRedGreen(glm::vec2 rg)
{
    sceneObjs[toolObj].rgb[0]+=rg[0];
    sceneObjs[toolObj].rgb[1]+=rg[1];
}

static void adjustBlueBrightness(glm::vec2 bl_br)
{
    sceneObjs[toolObj].rgb[2] += bl_br[0];
    sceneObjs[toolObj].brightness += bl_br[1];
}

static void lightMenu()
{
    if (ImGui::MenuItem("Move Light 1"))
    {
        deactivateTool();
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
            adjustBrightnessY, glm::mat2(1.0, 0.0, 0.0, 10.0));
    }

    if (ImGui::MenuItem("R/G/B/All Light 1"))
    {
        deactivateTool();
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, glm::mat2(1.0, 0, 0, 1.0),
            adjustBlueBrightness, glm::mat2(1.0, 0, 0, 1.0));
    }

    if (ImGui::MenuItem("Move Light 2"))
        deactivateTool();

    if (ImGui::MenuItem("R/G/B/All Light 2"))
        deactivateTool();
}

static void createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int))
{
    int nSubMenus = (size - 1) / 10 + 1;

    for (int i = 0; i < nSubMenus; i++) {
        char num[6];
        sprintf_s(num, sizeof(num), "%d-%d", i * 10 + 1, std::min(i * 10 + 10, size));

        if (ImGui::BeginMenu(num)) {
            for (int j = i * 10 + 1; j <= std::min(i * 10 + 10, size); j++)
                if (ImGui::MenuItem(menuEntries[j - 1]))
                    menuFn(j);

            ImGui::EndMenu();
        }
    }
}

static void materialMenu()
{
    if (ImGui::MenuItem("R/G/B/All")) {
        deactivateTool();
        if (currObject >= 0) {
            toolObj = currObject;
            setToolCallbacks(adjustRedGreen, glm::mat2(1, 0, 0, 1),
                adjustBlueBrightness, glm::mat2(1, 0, 0, 1));
        }
    }

    if (ImGui::MenuItem("UNIMPLEMENTED: Ambient/Diffuse/Specular/Shine")) {
        deactivateTool();
        // UNIMPLEMENTED
    }

    // You'll need to fill in the remaining menu items here.
}

static void adjustAngleYX(glm::vec2 angle_yx)
{
    sceneObjs[currObject].angles[1]+=angle_yx[0];
    sceneObjs[currObject].angles[0]+=angle_yx[1];
}

static void adjustAngleZTexscale(glm::vec2 az_ts)
{
    sceneObjs[currObject].angles[2]+=az_ts[0];
    sceneObjs[currObject].texScale+=az_ts[1];
}

static void mainmenu()
{
    deactivateTool();

    if (ImGui::MenuItem("Rotate/Move Camera")) {
        deactivateTool();
        doRotate();
    }

    if (ImGui::MenuItem("Position/Scale")) {
        deactivateTool();
        if (currObject >= 0) {
            toolObj = currObject;
            setToolCallbacks(adjustLocXZ, camRotZ(),
                adjustScaleY, glm::mat2(0.05, 0, 0, 10));
        }
    }

    if (ImGui::BeginMenu("Add object")) {
        createArrayMenu(numMeshes, objectMenuEntries, objectMenu);
        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Rotation/Texture Scale")) {
        deactivateTool();
        if (currObject >= 0)
            setToolCallbacks(adjustAngleYX, glm::mat2(400, 0, 0, -400),
                adjustAngleZTexscale, glm::mat2(400, 0, 0, 15));
    }

    if (ImGui::BeginMenu("Material")) {
        materialMenu();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Texture")) {
        createArrayMenu(numTextures, textureMenuEntries, texMenu);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Ground Texture")) {
        createArrayMenu(numTextures, textureMenuEntries, groundMenu);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Lights")) {
        lightMenu();
        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("EXIT"))
        exit(0);
}

static void makeMenu()
{
    if (ImGui::IsMouseReleased(HZ_MOUSE_BUTTON_RIGHT))
    {
        ImGui::OpenPopup("Main Menu");
    }

    if (ImGui::BeginPopupContextItem("Main Menu")) {
        mainmenu();
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------

void reshape( int width, int height )
{
    windowWidth = width;
    windowHeight = height;

    glViewport(0, 0, width, height);

    // You'll need to modify this so that the view is similar to that in the
    // sample solution.
    // In particular: 
    //     - the view should include "closer" visible objects (slightly tricky)
    //     - when the width is less than the height, the view should adjust so
    //         that the same part of the scene is visible across the width of
    //         the window.

    float nearDist = 0.2f;
    projection = glm::frustum(-nearDist * (float)width / (float)height,
        nearDist * (float)width / (float)height,
        -nearDist, nearDist,
        nearDist, 100.0f);
}

bool timer(float msecs)
{
    displayTimer_msecs += msecs;

    if (displayTimer_msecs > 1000.0f)
    {
        sprintf_s(title, sizeof(title), "%s %s: %d Frames Per Second @ %d x %d",
            lab, programName, numDisplayCalls, windowWidth, windowHeight);

        numDisplayCalls = 0;
        displayTimer_msecs = 0.0f;
        return true;
    }

    return false;
}

using namespace GLCore;
using namespace GLCore::Utils;

SandboxLayer::SandboxLayer(uint32_t width, uint32_t height)
    : m_Shader { nullptr }
{
    windowWidth = width;
    windowHeight = height;
}

SandboxLayer::~SandboxLayer()
{
}

void SandboxLayer::OnAttach()
{
	EnableGLDebugging();

    // Load shaders and use the resulting shader program
    m_Shader = Shader::FromGLSLTextFiles(
        "assets/shaders/vert.glsl",
        "assets/shaders/frag.glsl"
    );

    glUseProgram(m_Shader->GetRendererID());
    shaderProgram = m_Shader->GetRendererID();

    init();
    reshape(windowWidth, windowHeight);
}

void SandboxLayer::OnDetach()
{
	// Shutdown here
}

void SandboxLayer::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    
    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent& e) {
        reshape(e.GetWidth(), e.GetHeight());
        return false;
        });
    dispatcher.Dispatch<MouseMovedEvent>([&](MouseMovedEvent& e) {
        if (currButton == -1)
            mousePassiveMotion(e.GetX(), e.GetY());
        else
            doToolUpdateXY(e.GetX(), e.GetY());
        return false;
        });
    dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e) {
        mouseClickOrScroll(e.GetMouseButton(), true, e.GetMods(), mouseX, mouseY);
        return false;
        });
    dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e) {
        mouseClickOrScroll(e.GetMouseButton(), false, e.GetMods(), mouseX, mouseY);
        return false;
        });
    dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& e) {
        if (e.GetYOffset() > 0) // scroll up
            mouseClickOrScroll(3, true, 0, mouseX, mouseY);
        else // scroll down
            mouseClickOrScroll(4, true, 0, mouseX, mouseY);
        return false;
        });
}

void SandboxLayer::OnUpdate(Timestep ts)
{
    // Render here
    display();

    if (timer(ts.GetMilliseconds()))
    {
        auto& window = Application::Get().GetWindow();
        window.SetTitle(title);
    }
}

void SandboxLayer::OnImGuiRender()
{
    // ImGui here
    makeMenu();
}
