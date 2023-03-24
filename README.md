# Shitty Renderer

A Vulkan Renderer written in C++ 23. Very much WIP.

![Readme Image](docs/readme.png)

Control movement via Splines that minimize rotation via bishops frames!

<https://user-images.githubusercontent.com/8939023/225312673-ae9748e1-c363-45c1-ad7c-fdabec51e73d.mp4>

## Building

To build on Windows install the Vulkan SDK from <https://vulkan.lunarg.com/sdk/home#windows> and set the environment variable `VULKAN_HOME` to the install location.

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
- [x] Load Lost Empire
  - [x] Current memory usage is crazy, figure out what is causing that.
    - Around 349mb for 'lost_empire-RGB.png'.
    - Compression is needed, probably KTX.
    - It can be added by following the information in <https://github.com/KhronosGroup/Vulkan-Samples/blob/main/third_party/CMakeLists.txt>
  - [x] Textures are uploaded multiple times.
- [x] Add wireframe mode which can be switched to.
- [ ] Change from using a single primary command buffer to multiple secondary command buffers
  - There are resources for how to do this at:
    - <https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/>
    - <https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/performance/command_buffer_usage>
    - <https://vkguide.dev/docs/extra-chapter/multithreading/>
  - These secondary command buffers could be recorded in parallel in systems.
- [ ] Stop blocking when uploading textures, there is currently a wait on idle which is unnecessary.
  - Should probably also use the transfer queue.
  - Usage of transfer queue is only possible for some commands.
    - Therefore, a dedicated upload command is needed, which is separate from the mip map calculation.
    - Through another possibility is using multiple graphics queues. But the transfer queue might be faster so I need to look in to that.
  - Could be done in the asset manager and the handle could be checked to see if they are ready.
- [ ] Create an Asset Library that makes resources ready for the engine. Should include loading and storing of binary data. Reading the data is the responsibility of the program.
  - <https://vkguide.dev/docs/extra-chapter/asset_system/>
  - [ ] Meshes
  - [ ] Textures
    - [ ] MipMaps
- [x] Add a deletion queue instead of cleanup.
- [ ] Update to use Synchro2. <https://www.khronos.org/blog/vulkan-timeline-semaphores>
- [x] Incorporate changes found by Charles
  - <https://github.com/Overv/VulkanTutorial/issues/202>
  - <https://github.com/Overv/VulkanTutorial/pull/255>
- [x] Fix controls after adding Imgui
  - [x] Only capture mouse clicks if not over ui.
- [x] ~~Yoink list from <https://github.com/knightcrawler25/GLSL-PathTracer> to choose what objects should be manipulated by ImGuizmo~~
  - I just use a ray to select instead.
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
- [x] Implement Compute shaders <https://github.com/Overv/VulkanTutorial/pull/320>
- [x] Delete `Renderer::uploadBuffer` it should be handled by `AssetManager`
- [x] Create multiple vk::Framebuffer swapChainFramebuffers.
  - No don't do that, i made the change because it is imageless now.
- [x] Move cmake for third party to the folder containing them.
- [ ] Fix sync issues <https://github.com/Overv/VulkanTutorial/issues/276>
- [x] Fix cursor jumping bug due to ImGui getting input from another thread than GLFW. Need to sync IO somehow, someone on the Vulkan Discord Suggested modeling it as Producer/Consumer problem.
  - Might not be worth it to have more threads as they still need to sync.
- [ ] Add a way to control spline normal from both ends.
  - Might be possible to add normals from both ends and then interpolate between the two linearly along the path.
  - <https://gamedev.stackexchange.com/questions/94098/controlling-roll-rotation-when-travelling-along-bezier-curves>
  - Will probably be a trade off between given the user control and how jank it can look if they mess it up.
- [ ] deform meshes along spline. <https://stackoverflow.com/questions/69208203/bend-a-mesh-along-spline>
- [ ] Rename files and introduce a namespace.
  - Physics, Spline and Path are terrible naming right now. Also BEH is bad prefix
- [ ] Pre-dock debug info in ImGui, probably with a `DockBuilder`.
- [ ] Size the debug windows to the size of their content.
- [ ] Use shared_ptr reference counting in AssetManager to clean up unused assets when space is low.
- [ ] Move loading of models and textures to a class which can have multiple `loaders`.
  - Each loader has a `canHandle` and `handle` method that returns either a material or a mesh depending on the type of loader.
- [ ] Implement clickable control objects, which don't collide. This should be possible with <https://pybullet.org/Bullet/BulletFull/classbtGhostObject.html#details>
  - Should be tied together with the spline
- [ ] Make a component for if a entity is allowed to be manipulated with the gizmo and which ones.
- [ ] There is probably a lot of memory leaks from using new in relation to physics.
- [ ] Rework the spline class, it should only compute once it is dirty which should only happen on changes in the transform.
- [ ] Sensors should not scale. But there should still be a way to scale the control point.
- [ ] Maybe all buffers should be typed.
  - Has the problem that if I want to use buffer as a different type I can not. Which can be a problem.
- [ ] Fix minimize on windows
  - `Validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageExtent-01689 ] Object 0: handle = 0x16f47f34660, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0x13140d69 | vkCreateSwapchainKHR(): pCreateInfo->imageExtent = (0, 0) which is illegal. The Vulkan spec states: imageExtent members width and height must both be non-zero (https://vulkan.lunarg.com/doc/view/1.3.224.1/windows/1.3-extensions/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageExtent-01689)`
- [ ] RenderObject should not be used a component, Need a different wrapper for that
  - Which means that the transform sent to the engine should be given from the `Transform` component
- [ ] Transform sources should be mutually exclusive, which I don't know if I can check.
- [ ] Fix collisions so that they are the same size as the model.
- [ ] Make a input event enum, so there is reliance on the key pressed later.
- [ ] Maybe make a way to have systems depend on each other so they can run in order and can be sorted?
  - Could be an abstract class with a setup(registry) function, update and dependencies.
  - Then have a enum of all dependencies and use dependency sorting to call them in the correct order.
- [ ] Figure out where Components should be placed.
- [ ] Maybe delete players if they disconnect?
- [ ] Add a way to have read/write without it impacting of it's execution.
  - Maybe it is possible with a struct as a template variable <https://brevzin.github.io/c++/2019/12/02/named-arguments/>

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
auto [mesh, material] = 
  MeshBuilder::begin(renderer)
    .loadObj("filename.obj") // Maybe this is just load Mesh? Then delegate to the correct loader
    .loadMtl() // tinyObj loads the materials if referenced in the obj file
    .upload(); // upload to the GPU either now or later. 
```

When the ECS system gets added these handles could then be held together in a Mesh Component.

Using would also avoid callbacks as the renderer would just ignore meshes and materials that have not been uploaded yet, as the ECS view will not take them.
This would simplify the interaction model.

Maybe the engine returns futures, this could then be checked for results in the ECS and swapped for the result when ready.

```cpp
std::future<UploadedMesh> uploadMesh();

registry.view<std::future<UploadedMesh>>.each([&](auto entity, auto& result) {
    if(result.valid()) {
      registry.emplace<UploadedMesh>(entity, result.get());
      registry.remove<std::future<UploadedMesh>>(entity); // This is safe according to the documentation
    }
  });
```

The future wait could probably even templated and added as a list of Functions as to make it easy to extend.

### Physics multithreading idea

Need to separate the physics out to a separate thread, which means that transforms from previous iteration should be available.
These can be saved to a data structure and copied to a synced reference when ready.
Which means that the lock only happens while changing the reference.
The data should only be the transforms at the start, but might have AABB later for frustum culling.
The start up an update should be getting this reference, which probably means keeping it in a shared pointer.
Probably `atomic<shared_ptr<map<id, transform>>>`, then have the physics component just have the id for the index.

```cpp
struct PhysicsComponent { // Maybe just Physics? Check what other engines use as naming.
  uint64_t id; // Maybe typedef this.
};
```

Each of the updates that need the transforms can then get it passed it in the call to their update.

```cpp
void update(float delta) {
  auto transforms = world->getLatestTransforms();
  aiSystem->update(delta, transforms, jobs);
  objectSystem->update(delta, transforms, jobs);
  particleSystem->update(delta, transforms, jobs);
  world->step(delta, jobs);
  jobs->waitAll(); // Wait for all jobs which the systems create to be done.
}
```

This means that they all use the same transforms and thereby gives consistent results.

To allow for doing tasks which rely on `btDynamicWorld`, tasks can be dispatched to world which is executed before stepping.
These tasks would then create messages or call callbacks to give the information back later.
Tasks could be: CreateBody, DeleteBody or RayTest.
To make it easy to implement this a callback should probably just be used at the start with documentation saying they are not thread-safe.
