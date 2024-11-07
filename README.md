# Simple Scene Editor OpenGL
A university computer graphics project problem and question set. 

## Usage
```
git clone --recursive https://github.com/nessarus/SimpleSceneEditorOpenGL
```

Run `scripts/Win-Premake.bat` and open `OpenGL-Sandbox.sln` in Visual Studio 2019. `OpenGL-Sandbox/src/SandboxLayer.cpp` contains the example OpenGL code that's running.

* Starting point, inspect the skeleton files SandboxApp.cpp, SandboxLayer.h and SandboxLayer.cpp in the project src subdirectory. The project requires you to use the mouse to move objects and light sources around. 
* Pre-compiled assimp-5.3.1 library files have been used for this project. You can compile the assimp library from
source here (http://www.assimp.org/). This library that allows us to import various well-known 3D model formats.

## Files provided
* vert.glsl and frag.glsl - these are the vertex shader and fragment shader for scene-start.cpp.

* gnatidread.h - you shouldn't need to modify the code in this file, but feel free to.

* models-textures - this is a subdirectory containing many small images serving as texture maps for the project.

* SandboxApp.cpp, SandboxLayer.h and SandboxLayer.cpp - these are the main cpp program that you need to expand for the project. 

## Tasks
Implement all the functionality described in the items A to J (K optional).

### Task A
Modify the display function so that the camera rotates around the centre point using the variables camRotUpAndOverDeg and camRotSidewaysDeg. The variables camRotUpAndOverDeg and camRotSidewaysDeg are already set appropriately when moving the mouse with the left button down, or the middle button down (or shift + left button).
* When the left mouse button is held down and dragged horizontally across the window, the scene should rotate
about the axis vertical to the ground plane.
* When the left mouse button is held down and dragged vertically up (or down), the scene should zoom into (or out of) the scene.
* Dragging the middle mouse button horizontally is the same as dragging the left button horizontally.
* Dragging the middle mouse button vertically up (or down) across the window should change the
elevation angle of the camera looking at the ground plane up (or down).

### Task B
Modify the drawMesh function so the X, Y and Z axes angle rotations of the objects contained in the angles array member of the SceneObject structure. 
* Dragging the left mouse button vertically across the window should rotate the current object about the
x-axis (parallel to the ground plane and pointing to the right).
* Dragging the middle mouse button horizontally across the window should rotate the current object
about the z-axis (parallel to the ground plane and pointing out of the screen).
* Dragging the left mouse button horizontally across the window should rotate the current object about
the y-axis (vertical to the ground plane).
* Dragging the middle mouse button vertically across the window should change the texture scale on the
object.

### Task C
Implement a menu item for adjusting the amounts of ambient, diffuse, specular and shine light. The shine value needs to be able to increase to 100. 
Modify the materialMenu and makeMenu functions so that they change the appropriate
members of the SceneObject structure. The code to use these to affect the shading via the
calculation for the light is already implemented in the skeleton code provided.
Follow the corresponding implementations of other similar menu items. Use the setToolCallbacks function which has four arguments which are pointers to four float's that should be modified when moving the mouse in the x and y directions while pressing the left button or the middle button (or shift + left button). After each callback function accepting a pair of (x,y) relative movements is a 2×2 matrix which can be used to scale and rotate the effect of the mouse movement vector. See the calls to setToolCallbacks in SandboxLayer.cpp for example. The setToolCallbacks function is defined in gnatidread.h.
* the ambient or diffuse light of the object can be interactively changed by dragging the left mouse button horizontally or vertically;
* the specular light and amount of shine can be interactively adjusted by dragging the middle mouse button horizontally or vertically.
* the position of the light source can be modified by dragging the mouse button. For this functionality, you are suggested to use
    * the left mouse button for changing the x- and z-coordinates
    * the middle mouse button for changing the y-coordinates of the light source's position

### Task D
In the skeleton code, triangles are "clipped" (not displayed) if they are even slightly close to the
camera. Fix the reshape callback function so that the camera can give more "close up" views of
objects.

### Task E
Modify the reshape function so that it behaves differently when the width is less than the height: basically whatever is visible when the window is square should continue to be visible as the width is decreased, similar to what happens if the window height is decreased starting with a square.

### Task F
Modify the vertex shader so that the light reduces with distance - aim for the reduction to be noticeable without quickly becoming very dark. This should apply to the first light.

### Task G
Move the main lighting calculations into the fragment shader, so that the directions are calculated for individual fragments rather than for the vertices. This should have a very noticeable effect on the way the light interacts with the ground (especially when the ground has a plain texture map), since the ground only has vertices at the corners of a coarse grid.

### Task H
Generally specular highlights tend towards white rather than the colour of the object. Modify the shader code so that the specular component always shines towards white, regardless of the texture and the colour assigned to the object.

### Task I
Add a second light to both the C++ code and the shaders. The second light should be directional, i.e., the direction that the light hits each surface is constant and independent of the position of the surface. In particular, use the direction from the second light's location to the origin as the direction of the light. For example, moving the light source upward should increase the y-component of the direction of the light source, making the light source behave like the mid-day sun. Like the first light source, use sceneObj[2] to store the coordinates and colours of the second light, and make it similar to the first light, including placing a sphere at the coordinates of the light.

### Task J
Design and implement two small, but non-trivial additional functionality. E.g., (1) allowing objects in the scene to be deleted, or (2) duplicated. (3) Adding a wave motion to the ground or (4) to the scene objects. (5) Adding a spot light to the scene and allowing its illumination direction to be changed interactively. (6) Varying colours over time. (7) Toggling on/off the global coordinate system on the ground. (8) Allowing the user to select another object as the current object.

### Task K
Add additional functionality. E.g., (1) loading and saving scenes, (2) clicking on objects to edit them, (3) setting paths for objects to move along over time, (4) motion under physics, (5) particle effects, (6) attaching one object to another so that they move and rotate together.