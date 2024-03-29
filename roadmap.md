# Antilegacy editor roadmap
**This is a simple development roadmap for the antilegacy editor**  
It lists minimal editing functionality for the editor to be considered a finished project

## Global

- [ ] Editor
  - [ ] Resource library
  - [ ] Event history
    - [ ] Undo feature
  - [ ] Snap to grid feature
  - [ ] Shortcut config feature

- [ ] IO
  - [x] Import model
  - [ ] Create scene file
  - [ ] Serialize scene file
  - [ ] Export model
  - [x] Import resources
    - [x] Textures
    - [ ] Rigs
    - [x] Meshes

- [ ] View
  - [ ] Camera movement
    - [ ] Direction
      - [x] free rotation
      - [x] Focus on model
      - [ ] Fix camera direction along global XYZ
      - [x] manually set angle
    - [ ] Movement
      - [x] free movement
      - [x] along global XYZ
      - [ ] along local XYZ
      - [ ] fix camera position
      - [x] manually set rotation
    
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
    - [x] Direct
    - [ ] Diffuse
  - [ ] configure lights
    - [ ] Intencity 
    - [ ] Diffusion
    - [ ] Source size
    - [x] Position
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
  - [x] Gizmos
  - [ ] Debug

## Backlog
- [ ] Editor API for multiple renderers
