# Antilegacy editor roadmap
*This is a simple development roadmap for the antilegacy editor*

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
  - [ ] Import resources
    - [ ] Textures
    - [ ] Rigs
    - [ ] Meshes

- [ ] View
  - [ ] Camera movement
    - [ ] Direction
      - [x] free rotation
      - [x] Focus on model
      - [ ] Fix camera direction along global XYZ
      - [x] manually set angle
    - [ ] Movement
      - [ ] free movement
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