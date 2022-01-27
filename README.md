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
  - [ ] Depth buffering
  - [ ] Loading Models
  - [ ] Generating Mipmaps
  - [ ] Multisampling
- [x] Copy Textures to output folder automatically
- [ ] Move Model Matrix into push constant, but leave proj and view in UBO
- [ ] Load more objects
  - [ ] Introduce VMA, VK-Guide has a tutorial for this. Else the buffer management is too complex
- [ ] Add wireframe mode which can be switched to.
- [ ] Change from using a single primary buffer to multiple secondary buffers
  - There are resources for how to do this at:
    - <https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/>
    - <https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/performance/command_buffer_usage>
    - <https://vkguide.dev/docs/extra-chapter/multithreading/>
- [ ] Stop blocking when uploading textures, there is currently a wait on idle which is unnecessary.
  - Should probably also use the transfer queue.
