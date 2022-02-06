# VulkanTut

Learning Vulkan from <https://vulkan-tutorial.com>, while trying to keep it as clean and modern c++ as possible.

## Compiling on Windows

To compile on Windows download Vulkan SDK from <https://vulkan.lunarg.com/sdk/home#windows>

## TODO

- [ ] Finish chapters of Vulkan-Tutorial
  - [x] Texture Mapping
    - [x] Images
    - [x] Image view and sampler
    - [x] Combined image sampler
      - [x] Fix gamma problem
      - [x] Fix flipped problem
  - [x] Depth buffering
    - [ ] Fix the problems present in the chapter: <https://stackoverflow.com/questions/62371266/why-is-a-single-depth-buffer-sufficient-for-this-vulkan-swapchain-render-loop>
  - [x] Loading Models
  - [x] Generating Mipmaps
  - [x] Multisampling
- [x] Copy Textures to output folder automatically
  - [x] Add it as custom command so it is done on build
- [x] Move Model Matrix into push constant, but leave proj and view in UBO
- [ ] Load more objects
  - [x] Introduce VMA, VK-Guide has a tutorial for this. Else the buffer management is too complex
    - [ ] uploadIndices and uploadVertices can be generalized and simplified.
    - [ ] TextureImage and UniformBuffer still uses manual buffer creation. They should be moved over.
- [ ] Add wireframe mode which can be switched to.
- [ ] Change from using a single primary buffer to multiple secondary buffers
  - There are resources for how to do this at:
    - <https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/>
    - <https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/performance/command_buffer_usage>
    - <https://vkguide.dev/docs/extra-chapter/multithreading/>
- [ ] Stop blocking when uploading textures, there is currently a wait on idle which is unnecessary.
  - Should probably also use the transfer queue.
- [ ] Create a Asset Library that makes resources ready for the engine. Should include loading and storing of binary data. Reading the data is the responsibility of the program.
  - [ ] Meshes
  - [ ] Textures
    - [ ] MipMaps
- [ ] Add a deletion queue instead of cleanup.
- [ ] Update to use Synchro2. <https://www.khronos.org/blog/vulkan-timeline-semaphores>
- [ ] Incorporate changes found by Charles
  - <https://github.com/Overv/VulkanTutorial/issues/202>
  - <https://github.com/Overv/VulkanTutorial/pull/255>
