# GLB Load Capture
Loads .glb foramt files, render as IBL method and capture in various angles.
## Tech spec
ISO C++ 14 standard
## Dependencies
- [GLM](https://github.com/g-truc/glm): Calculations for OpenGL
- [GLFW](https://glfw.org/): Create window, context
- [GLAD](https://glad.dav1d.de/): Load functions
- [Assimp](https://assimp.org/): Load 3d models
- [OpenCV](https://opencv.org/releases/): Image processing
- [`stb_image.h`](https://github.com/nothings/stb): Load image files and proccess
## Before Execution
- Check if all the links for the external libraries are set properly.
  - You might need .dll files for 'OpenCV' and 'Assimp'.
  - Rebuild in your environment if needed.
- Check string variables below for your paths.
  - modelsLoc
  - hdrLoc
  - savePath
  - ~~textureLoc~~
- Result images will be saved in the directories of your 'savePath'.
## Notes
Almost of the codes for the rendering refer to 'Learn OpenGL'.
- [IBL rendering - Learn OpenGL](https://learnopengl.com/PBR/IBL/Diffuse-irradiance)
