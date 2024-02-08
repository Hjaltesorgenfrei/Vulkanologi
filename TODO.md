# Releasable game

Create a game that can be released on steam and sold.

## Priority List

Remove tasks and tick them in the todo as they get done.

1. Jitter Buffer
2. Car Settings in debug menu
3. Car Feel, get help here
4. Client can affect state
5. Player-Player collision

## Code Articeture

- [ ] Create a `World` struct that keeps the world and can be passed around to access stuff entities and physics.
  - PhysicsWorld
  - CurrentFrame
  - Network to send messages
- [ ] Save and load struct from json
- [ ] Restructure Mesh handling as described in [Design.md](./DESIGN.md)

## Rendering

- [ ] Shadows
- [ ] Lights

## Networking

- [ ] Jitter Buffer
- [ ] Client can affect state
- [ ] Player-Player collision
- [ ] Client view is set by server
- [ ] State synchronization by priority

## Gameplay Related

- [ ] Car Feel, get help here
  - [ ] Torque / Weight is a bit weird right now, as the weight depends on size of the model.
  - [ ] Car wheels should be placed by model, maybe using the names of the meshes. Like fl, fr, rl, & rr.
  - [ ] Size and width of the wheel should also be determined by the model
- [ ] Camera controls
  - [ ] Switch between cameras

## Debugging

- [ ] Car Settings in debug menu
- [ ] Physics Debug view

## Level Design

- [ ] At least a few levels
  
## 3D Assets

- [ ] Car
- [ ] Wheels

## User Interface

- [ ] Main Menu
- [ ] Options Menu
- [ ] In-game Ui

## Level Editor

- [ ] Save/Load
- [ ] Spawn entity
- [ ] Snap objects
- [ ] Directory view of models
  - [ ] Thumbnails

## Sound

- [ ] Engine Start
- [ ] Engine Stop
- [ ] Breaking
- [ ] Engine at different rpm
- [ ] Exhaust
- [ ] Horn
- [ ] Tire road noise sound
- [ ] Tires turning when not driving
- [ ] Wind noise at speed

## Steam Related

- [ ] Join friends
- [ ] Serialize yojimbo messages to steam network messages.
- [ ] Use `ISteamNetworkingSockets::CreateListenSocketP2P`

## Unsorted and maybe outdated

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
- [x] Create a style to autoformat with
  - [x] Format all files.
- [ ] Move swap chain to its own class
- [x] Fix the hash for checking if vertices are equal.
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
  - Interpolating between the two quaternion from each end should work.
- [x] deform meshes along spline. <https://stackoverflow.com/questions/69208203/bend-a-mesh-along-spline>
- [ ] Rename files and introduce a namespace.
  - Physics, Spline and Path are terrible naming right now. Also BEH is bad prefix
  - [ ] Use name "Rotte" for everything.
    - Repo should be called "Rotte Engine" and namespace "rotte"
- [ ] Pre-dock debug info in ImGui, probably with a `DockBuilder`.
- [ ] Size the debug windows to the size of their content.
- [ ] Use shared_ptr reference counting in AssetManager to clean up unused assets when space is low.
  - Reference count of 1 means it can be cleaned.
- [ ] Move loading of models and textures to a class which can have multiple `loaders`.
  - Each loader has a `canHandle` and `handle` method that returns either a material or a mesh depending on the type of loader.
- [x] Implement clickable control objects, which don't collide. This should be possible with <https://pybullet.org/Bullet/BulletFull/classbtGhostObject.html#details>
  - Should be tied together with the spline
- [ ] Make a component for if a entity is allowed to be manipulated with the gizmo and which ones.
- [ ] There is probably a lot of memory leaks from using new in relation to physics.
  - This might have been fixed when i moved to Jolt. Need to check first
- [ ] Rework the spline class, it should only compute once it is dirty which should only happen on changes in the transform.
- [ ] Sensors should not scale. But there should still be a way to scale the control point.
- [ ] Maybe all buffers should be typed.
  - Has the problem that if I want to use buffer as a different type I can not. Which can be a problem.
- [ ] Fix minimize on windows
  - `Validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageExtent-01689 ] Object 0: handle = 0x16f47f34660, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0x13140d69 | vkCreateSwapchainKHR(): pCreateInfo->imageExtent = (0, 0) which is illegal. The Vulkan spec states: imageExtent members width and height must both be non-zero (https://vulkan.lunarg.com/doc/view/1.3.224.1/windows/1.3-extensions/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageExtent-01689)`
- [ ] RenderObject should not be used a component, Need a different wrapper for that
  - Which means that the transform sent to the engine should be given from the `Transform` component
- [ ] Transform sources should be mutually exclusive, which I don't know if I can check.
  - So that any single entity only has one. Else it gets confusing about where it should be.
- [ ] Fix collisions shapes so that they are the same size as the model.
- [x] Make a input event enum, so there is reliance on the key pressed later.
- [x] Maybe make a way to have systems depend on each other so they can run in order and can be sorted?
  - Could be an abstract class with a setup(registry) function, update and dependencies.
  - Then have a enum of all dependencies and use dependency sorting to call them in the correct order.
- [ ] Figure out where Components should be placed.
- [x] Maybe delete players if they disconnect?
- [x] Add a way to have read/write without it impacting of it's execution.
  - Maybe it is possible with a struct as a template variable <https://brevzin.github.io/c++/2019/12/02/named-arguments/>
- [ ] Remove the need for `Others<>` in the system type.
  - Probably just by making multiple classes that do the same thing.
  - I don't actually know if this is a good idea. It can be used for coordinating systems.
    - So might be useful to keep around a while longer.
- [x] Add a background cube map, the black is getting annoying.
- [x] Investigate the jittering issue from the vehicle driving around. A post on the Bullet forums mentioned it possibly being due to large bodies.
- [ ] Make spline deformation better.
  - [ ] Use the total arc length to create N meshes, where N ~= (arc length)/(mesh length)
  - [ ] Add a scale on y and z axis for scaling the meshes. It should be linearly interpolated for between control points.
  - [ ] Test with more than 2 control points.
  - [ ] Actually upload the damn thing each frame
- [ ] Add buffers for often updated geometry, the current method of writing all the data each frame is horribly slow.
  - Care needs to be taken when and how they are deleted/written to, as a currently used buffer is considered off limits.
  - Possibly have SwapChainImages persistently mapped buffers for each kind of this data. Like I have for lines.
    - Then just a call to update them which updates them over the next N frames before they are used.
    - Also some way to only delete part of it probably.
      - Malloc, Alloc style.
    - <https://stackoverflow.com/questions/62182124/most-generally-correct-way-of-updating-a-vertex-buffer-in-vulkan>
  - Another way to do this for just the splines would be to index in a buffer of transforms instead of using the push constant.
    - This would mean that the interpolation happens on the GPU and only the transforms need to uploaded.
- [ ] Add some better ambient light <https://learnopengl.com/Lighting/Basic-Lighting>
- [ ] A way to decide what a Sensor is allowed to collide with, maybe by it's tag.
  - Also maybe adding a function that is executed upon collision would be nice.
  - The Jolt api for start collision and stop is a bit wonk due to sleep.
- [x] Add to the systems that they may write to other registers without getting them as parameters.
  - Can be done with `Others<>`
- [x] Figure out why ray cast vehicle glitches through the ground.
  - Was due to the wheel ray cast origin not being inside the collision shape.
- [ ] Make a collision editor and a serialization system for collision shapes.
- [x] Maybe make a change to btRayCastVehicle so it uses btMultiBody and add that moving platforms should add speed to the wheels.
  - This change could be upstreamed <https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=6365>
  - Also it has a todo in the code to add driving on moving platforms.
  - Lol never mind, changed to a different physics library.
- [ ] Make a precompiled header file, which includes all the files i want to precompile include. Then just include that file everywhere?
  - Need to look up how this is best done.
- [ ] Add lateral and longitudinalFriction to the car physics.
- [ ] Create debugging UI for the Car which allows for changing its settings while running.
  - `VehicleConstraint::GetConstraintSettings()` is not implemented and it is marked const.
  - So I probably need to recreate the constraint if it is changed.
- [ ] Physics should be rendered for next frame and interpolated towards which should reduce stuttering.
- [ ] Probably set the target position and target look in the camera and call update later.
- [ ] Make different kind of camera controllers which takes input and controls the camera.
  - [ ] Free fly, Just takes keyboard.
  - [ ] Car follow, takes a Car state as input. Probably Needs to be a entity pointer as it might be on another entity.
    - Can probably just be a physics object it follows in this case. It also has a velocity.
    - Keyboard should also allow for rotating the camera around the object in any case.
  - Might be an idea to allow multiple cameras to exists on many objects or more on just one. And then just switch between them.
- [ ] Possibly move adding/onRemove to Physics file to make sure that the correct components are added and removed.
- [ ] C# scripting could probably be done with .NET hosting. <https://github.com/dotnet/samples/tree/main/core/hosting>
  - Mono is another choice, <http://nilssondev.com/mono-guide/book/>
    - <https://github.com/lambdageek/monovm-embed-sample>
      - Check the forks if someone has something better first.
  - DotNetEvolution recommended just using mono. An integration can be found in <https://github.com/crazytuzi/UnrealCSharp>
- [ ] Separate parts out in separate projects
  - [x] Rendering
  - [ ] Client
  - [ ] Asset Loading
  - [x] Shapes and Curves Library
  - [ ] Make a CMakeLists.txt for resources and shaders.
- [ ] Make a editor UI for scenes.
  - <https://github.com/jmorton06/Lumos> has something similar to what I want to build.
  - Figure out how this should work with the swapchain and renderpass.
    - Either just render to an image that gets shown in ImGui or else it might be possible to simple only render part of the screen.
      - Only rendering to part of it, would avoid recreating the swapchain which can be very slow.
  - [ ] Make some way to debug entities, so that every (or just some) components can be shown in the ui.
    - [ ] Could be by registering Typed commands for each component
    - [ ] And base functions which handles each other type.
    - Lumos also does this
    - <https://github.com/Green-Sky/imgui_entt_entity_editor>
- [ ] Figure out where in the Yojimbo loops that messages should be received. Current method can crash.
- [ ] There seems to be a bug when a box interacts with mesh shape at a sharp angle which causes a crash.
- [ ] Change the camera to have distance and such as saved fields. So Only the position, rotation and position is sent.
  - [ ] The camera should then be called update on to allow it to ease.
- [ ] Fix the camera so it does not point in a nonsense direction at the start.
- [x] Steal the dependency updating from <https://github.com/liblava/liblava>.
  - [x] Use CPM instead
  - [x] A "lock" file
  - [x] Script to update all versions
  - [x] json file to add repos and tags.
- [ ] Implement t3ssel8r's shader: <https://imgur.com/gallery/qwhbHQq>
  - <https://www.reddit.com/r/godot/comments/14sxi4h/i_have_refined_my_water_shader/>
  - <https://www.reddit.com/r/godot/comments/14as2b9/i_added_a_stable_subtexel_smooth_camera_to_my_3d/>
- [ ] CPM has some caching would should be configured to remove redundant downloads.
- [ ] Create a pipeline that benchmarks compile speed for windows and linux with the results as artifact.
- [ ] <https://poniesandlight.co.uk/reflect/island_rendergraph_1/>
- [ ] Look into libraries
  - [ ] <https://github.com/bombela/backward-cpp>
  - [ ] <https://github.com/tgfrerer/island> has hot reloading of stuff which i want :)
  - [ ] <https://github.com/cg-tuwien/Auto-Vk-Toolkit>
- [ ] Implement Meshlet support
- [ ] <https://poniesandlight.co.uk/reflect/island_rendergraph_1/>
- [ ] Add support for multiple cameras and tag the active one.
  - [ ] Draw debug lines showing its frustum or at least a wireframe camera.
- [ ] Move all the GLFW -> Entt input stuff to a system or at least a different file.
  - Application is too long and its confusing.
  - Having it in a system would mean a reference to the GLFW window might be duplicated. Which might present a problem later.
- [ ] Decide if Keyboard/Mouse Input should be a component or just a singleton.
  - There is only ever gonna be one.
  - But it does make some stuff simple.
- [ ] Make something that is in front of renderer, so we dont include a file which has a million imports.
- [ ] When the client is in, delete shared. It is unneeded and slows down compile.
- [ ] Try making a voice chat for fun. PortAudio might work for this.
- [ ] Objects deformed along beziers only look correct for the first.
  - Likely a problem in `Bezier::recompute`
    - evenTsAlongCubic does not consider there might be mutiple control points.
- [ ] Use ConvexShapes for cars as they are largely convex and should support dynamic collision.
- [ ] Use a layered scenes, where they point to the lower layer.
  - It can call down with draw and update calls. Which would allow for overlays and pausing.
  - Needs scenes in general to switch between what is loaded
  - Prefabs/Prototypes should be held in a different registry if they are used.
