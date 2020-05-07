# flux
A toy rendering engine. Sandbox for my experiments in computer graphics.

# Supported platforms:
* Windows 7 and older

# Supported graphics API:
* OpenGL 4.5

# Implemented graphics features:

* Stable cascaded shadow maps
* Physically based materials with Cook-Torrance BRDF (Unreal Engine approach mostly)
* Image based enviroment lightning (Unreal split-sum approximation)
* FXAA
* HDR with global filmic (approximated) tonemapping

# Implemented engine features:

* Asynchronous resource loading
* Simple scene serialization
* Simple editor
* Runtime code hot-reloading [1]
* Manually implemented win32 platform layer (definetly has tons of bugs)

# Some screenshots:

![Screenshot1](https://raw.githubusercontent.com/swarzzy/flux/master/screenshots/flux1.png)
![Screenshot2](https://raw.githubusercontent.com/swarzzy/flux/master/screenshots/flux2.png)

# How to build:

Just find and run vcvars.bat for x64 in your command promt and then go to project directory and run build.bat

# References:

1. Handmade hero: https://handmadehero.org/
2. Real-Time Rendering, Fourth Edition, Tomas Akenine-Möller, Eric Haines, Naty Hoffman, ISBN:  9781138627000
3. Shadow Mapping Summary – Part 1, 	Ignacio Castaño: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
4. Cascaded shadow maps, Jonatan Blow: http://the-witness.net/news/2010/03/graphics-tech-shadow-maps-part-1/
5. A Sampling of Shadow Techniques: https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/
6. Filmic tonemapping operators: http://filmicworlds.com/blog/filmic-tonemapping-operators/
7. Stephen Hill ACES approximation: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
8. OpenGL Insights: Asynchronous buffer transfer: https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-AsynchronousBufferTransfers.pdf
9. Moving Frostbite to Physically Based Rendering 3.0: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
10. Real Shading inUnreal Engine 4, Siggraph 2013: https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
11. Learn OpenGL (RUS): https://habr.com/ru/post/310790/, Original: www.learnopengl.com 
12. Unreal Engine 4 source
13. Crytec Sponza: https://www.alexandre-pestana.com/pbr-textures-sponza/
14. Room 3d model: https://sketchfab.com/3d-models/room-607d5a79f586497581a552407e7ef817
