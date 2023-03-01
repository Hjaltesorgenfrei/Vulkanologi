# Caldera

A Vulkan Renderer written in C++ 23. Very much WIP.

![Readme Image](docs/readme.png)

## Building

To build on Windows install the Vulkan SDK from <https://vulkan.lunarg.com/sdk/home#windows> and set the enviroment variable `VULKAN_HOME` to the install location. 

On arch install `sudo pacman -S vulkan-devel shaderc`

## Credits

Based upon [Vulkan Tutorial](https://vulkan-tutorial.com), inspiration drawn from [Vulkan Guide](https://vkguide.dev/) 
and [Brendan Galea's YouTube series](https://www.youtube.com/c/BrendanGalea).
Some ideas were also taken from [Zeux's blog](https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/).

## TODO

- [x] Finish chapters of Vulkan-Tutorial
  - [x] Texture Mapping
    - [x] Images
    - [x] Image view and sampler
    - [x] Combined image sampler
      - [x] Fix gamma problem
      - [x] Fix flipped problem
  - [x] Depth buffering
    - [x] Fix the problems present in the chapter: <https://stackoverflow.com/questions/62371266/why-is-a-single-depth-buffer-sufficient-for-this-vulkan-swapchain-render-loop>
  - [x] Loading Models
  - [x] Generating Mipmaps
  - [x] Multisampling
- [x] Copy Textures to output folder automatically
  - [x] Add it as custom command so it is done on build
- [x] Move Model Matrix into push constant, but leave proj and view in UBO
- [x] Load more objects
  - [x] Introduce VMA, VK-Guide has a tutorial for this. Else the buffer management is too complex
    - [x] uploadIndices and uploadVertices can be generalized and simplified.
    - [x] Descriptors need to be dynamically allocated, as they currently fill the pool.
      - <https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/>
      - Potentially just make a dynamic allocator which creates a pool for each type?
        - Then, when a descriptor set for textures is asked for, more can just be allocated. This would avoid any memory problems.
    - [x] TextureImage VMA.
    - [x] UniformBuffer still uses manual buffer creation. They should be moved over.
    - [x] It's possible to load an array of textures, this should make it easier to load different textures
      - <https://gist.github.com/NotAPenguin0/284461ecc81267fa41a7fbc472cd3afe>
- [ ] Load Lost Empire
  - [ ] Current memory usage is crazy, figure out what is causing that.
    - Around 349mb for 'lost_empire-RGB.png'.
    - Compression is needed, probably KTX.
  - [x] Textures are uploaded multiple times.
- [x] Add wireframe mode which can be switched to.
- [ ] Change from using a single primary buffer to multiple secondary buffers
  - There are resources for how to do this at:
    - <https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/>
    - <https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/performance/command_buffer_usage>
    - <https://vkguide.dev/docs/extra-chapter/multithreading/>
- [ ] Stop blocking when uploading textures, there is currently a wait on idle which is unnecessary.
  - Should probably also use the transfer queue.
  - Usage of transfer queue is only possible for some commands.
    - Therefore, a dedicated upload command is needed, which is separate from the mip map calculation.
    - Through another possibility is using multiple graphics queues. But the transfer queue might be faster so I need to look in to that.
- [ ] Create an Asset Library that makes resources ready for the engine. Should include loading and storing of binary data. Reading the data is the responsibility of the program.
  - <https://vkguide.dev/docs/extra-chapter/asset_system/>
  - [ ] Meshes
  - [ ] Textures
    - [ ] MipMaps
- [x] Add a deletion queue instead of cleanup.
- [ ] Update to use Synchro2. <https://www.khronos.org/blog/vulkan-timeline-semaphores>
- [ ] Incorporate changes found by Charles
  - <https://github.com/Overv/VulkanTutorial/issues/202>
  - <https://github.com/Overv/VulkanTutorial/pull/255>
- [x] Fix controls after adding Imgui
  - [x] Only capture mouse clicks if not over ui.
- [ ] Yoink list from <https://github.com/knightcrawler25/GLSL-PathTracer> to choose what objects should be manipulated by ImGuizmo
- [ ] Use VMA.hpp instead
- [ ] Create a style to autoformat with
- [ ] Move swap chain to its own class
- [ ] Fix the hash for checking if vertices are equal.
- [x] Fix that cast which are needed because of VMA, probably just put it in a class which has a hpp interface.
  - Currently makes it so new versions of MSVC cant compile the project.
  - Was caused by compiling for 32 bit, so don't do that.
- [ ] Abstract pipelines some more
  - Maybe create some systems which holds the pipeline and layout.
  - The systems could also have a render function.
- [ ] Create handles for materials and uploaded models, currently its just returned as an object. 
  - The handles could be `uint64_t`.
  - This would also remove the shared ptr, which is weird to use anyway as the lifetime of the object is more than the data.
- [x] Smooth resize
  - [x] Use old_swapchain when creating a new swapchain
  - [x] Never resize in the drawing thread, only call resize in the  OS 'resize callback/event'
  - [x] In the OS 'resize callback/even', repaint the scene.
  - [x] Never delete resources (images, views, framebuffers) in the resize. Instead put it in a delayed-deletion-queue. This will delete it when its no longer being used in a couple of frames
  - [x] use imageless framebuffer
  - [x] If the items in 4 and 5 are implemented, the resize function will not need to call vkDeviceWaitIdle()
- [ ] Implement Compute shaders <https://github.com/Overv/VulkanTutorial/pull/320>
- [ ] Delete `Renderer::uploadBuffer` it should be handled by `AssetManager`
- [ ] Create multiple vk::Framebuffer swapChainFramebuffers.

### Descriptor Layout Idea

The DescriptorSet setup could work by creating DescriptorSetLayouts before.
Then validating the created DescriptorSets against a provided layout. 
The DescriptorSetLayout could also have a builder
`.addBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, uint32t_t count = 1)`
If any binding has > 1 count, then it should be variable length and partially bound.
Only the last binding may have variable length, so this should also be checked. Maybe it should just be a flag?
[VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorBindingFlagBits.html)

Using a buffer for the other locations might be a possibility to still add all material properties as bindless. But that is to be looked at.

Then the DescriptorBuilder would have

```cpp
DescriptorBuilder &bindBuffer(uint32_t binding, vk::DescriptorBufferInfo *bufferInfo, vk::DescriptorType type);
DescriptorBuilder &bindImage(uint32_t binding, vk::DescriptorImageInfo *imageInfo, vk::DescriptorType type);
DescriptorBuilder &bindImages(uint32_t binding, std::vector<vk::DescriptorImageInfo>& imageInfos, vk::DescriptorType type);
```

bindImages would also validate that the length is less than the max set in descriptor builder.

### Handles

To simplify handling of resources it's probably easier if the interface of the renderer has Handles as return values.
So uploading a mesh returns a `MeshHandle` and similarly for a Materials.

```cpp
struct MeshHandle {
  uint64_t handle;
};
```

A builder could then be used to get these materials, as there is probably going to be quite a few ways to do it.
But this might require that the result of the build is a pair of `MeshHandle` and `Optional<MaterialHandle>`.
Do not really know how nice that would be.

```cpp
MeshBuilder::begin()
  .loadObj("filename.obj")
  .loadMtl() // tinyObj loads the materials if referenced in the obj file
  .build();
```

When the ECS system gets added these handles could then be held together in a Mesh Component.

Using would also avoid callbacks as the renderer could just ignore meshes and materials that have not been uploaded yet. 
This would simplify the interaction model.
