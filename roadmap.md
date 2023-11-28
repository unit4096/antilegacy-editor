# Antilegacy editor roadmap
*This is a simple development roadmap for the antilegacy editor*

## Current
- [x] move tinyobjloader and stbi to separate .cpp files
  - [x] tinyobj
  - [x] stbi
- [x] separate the vulkan renderer and the editor into different files
  - [x] remove dependencies between the renderer and the loader
- [x] add input handling using GLFW
- [ ] add ImGUI interface
  - [x] Add Demo code
  - [ ] Add handlers and gizmos
- [ ] rewrite the project using C++ modules
  - [x] renderer
  - [ ] loader

- [ ] renderer
  - [ ] add API for input
  - [ ] add more helpers to Vulkan functions
  - [ ] implement loading of multiple models to the renderer
  - [ ] implement runtime model loading/unloading

- [ ] loader
  - [ ] add GLTF model import
    - [x] mesh data
    - [x] textures
    - [ ] normals
    - [ ] PBR
  - [ ] make loader functions static

- [ ] logging
  - [ ] add basic logging functions
  - [ ] create wrappers to errors

- [ ] misc
  - [ ] add namespaces to ALE classes


## Global

- [ ] Editor
  - [ ] Resource library
  - [ ] Event history
    - [ ] Undo feature
  - [ ] Snap to grid feature
  - [ ] Shortcut config feature

- [ ] IO
  - [ ] Import model
  - [ ] Create scene file
  - [ ] Serialize scene file
  - [ ] Export model
  - [ ] Import resources
    - [ ] Textures
    - [ ] Rigs
    - [ ] Meshes

- [ ] View
  - [ ] Camera movement
    - [ ] Direction
      - [ ] free rotation
      - [x] Focus on model
      - [ ] Fix camera direction along global XYZ
      - [ ] manually set angle
    - [ ] Movement
      - [ ] free movement
      - [x] along global XYZ
      - [ ] along local XYZ
      - [ ] fix camera position
      - [ ] manually set rotation
    
- [ ] Mesh manipulation
  primitives -- vertices, edges, faces
  - [ ] Add primitives
  - [ ] Select primitives (using modes)
    - [ ] Singular
    - [ ] Multiple
    - [ ] Area
  - [ ] Rotate edges, faces
  - [ ] Change size of edge, face
  - [ ] Extrude face, set of faces
  - [ ] Split shape by edge
  - [ ] Mirror mesh
    - [ ] One axis
    - [ ] Two axis

- [ ] Textruing
  - [ ] Search textures
  - [ ] Add mesh materials
  - [ ] Generate UV layout
  - [ ] Updage UV layout
  - [ ] Apply texture
    - [ ] To model
    - [ ] To material
  
- [ ] Shader Previews

- [ ] Dynamic lighting
  - [ ] Create lights
    - [ ] Direct
    - [ ] Diffuse
  - [ ] configure lights
    - [ ] Intencity 
    - [ ] Diffusion
    - [ ] Source size
    - [ ] Position
    - [ ] Direction

- [ ] Rigging & skinning
  
  - [ ] Rigging
    - [ ] CRUD bones
    - [ ] Extrude bone
    - [ ] Mirror bones
  - [ ] Skinning
    - [ ] Generate weights
    - [ ] Modify weight values

- [ ] UI
  - [ ] Gizmos
  - [ ] Debug
  - [ ] 

## Backlog
- [ ] Editor API for multiple renderers
- [ ] 