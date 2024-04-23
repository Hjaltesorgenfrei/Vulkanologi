# Design Pondering

Ideas and todos i have for systems and implementations

## Descriptor Layout Idea

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

## Handles

To simplify handling of resources it's probably easier if the interface of the renderer has Handles as return values.
So uploading a mesh returns a `Mesh` and similarly for a Materials.
By listening to events where the struct is removed it should also be possible to reference count the meshes.
This would allow for the renderer to clean up unused meshes and materials.

```cpp
struct Mesh {
  const uint64_t handle;
};
```

The handle would probably do nothing until the mesh or material has been uploaded.
Loading a obj into cpu memory should return a load result inside the renderer, a struct containing the information.

```cpp
struct MeshLoadResult {
  std::vector<Vertex> vertices;
  std::vector<indices> indices;
  std::vector<std::string> materialPaths;
  uint32_t materialCount;
};
```

The communication, between the engine and renderer, of which materials was contained within the mesh should be done somehow.
Because it might be wanted to reuse the materials or swap them.
Alternatively this should be in a `AssetSystem` or something like that.
Then the amount of methods needed to query the names of assets and the like would be less of a problem.

Checking that a Mesh and it's material has the same amount of material indices would probably be a good idea.
If now some sort of error should be given.

A possibility is also using a `struct PendingMesh { std::string path };`
The renderer could listen to this event and swap it, but it might be less clear then just using a call.

## Physics multithreading idea

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

## Concurrent system graph execution

Note: This needs sync access to the registry

Because systems are executed in order of dependencies it should be possible to execute them concurrently.
Through care should be made to make sure that only one system can write to each type at a time.
This could be done by assigning an id to each write and other component when added to the system.
The ID could then be used in a set to make sure that each component is only being written to by one system at a time.
One efficient implementation of this would be a atomic bitset in which all currently write locks are held.
When a system starts all its writes component are locked by id and unlocked when the system finishes.
As only one system is allowed to write at a time for each component it can simple assign the rows to 0.
`boost::dynamic` is probably a good implementation to use for this as it is faster than `std::vector<bool>`.

Pseudo code for the process, the implementation actually uses int keys to avoid copying or using pointers.

```cpp

std::vector<SystemNode> systems;
std::vector<SystemNode> ready; 
std::vector<SystemNode> waitingOnWrites;
bitset writeLocks(countComponents(systems));

void startExecution(SystemNode system) {
  system.execute();
  system.informDependents();
  writeLocks &= (!system.writeBitSet()) // Remove all the write locks we took.
  runAllReady();
}

void runAllReady() {
  for (int i = 0; i < ready.size(); i++) {
    if (ready[i].writeBitSet() & writeLocks == false) { // No overlap in locks. Also probably needs to be a atomic assign with an bit and, as it will be checked from multiple threads. 
      theadPool.post(startExecution(ready[i]));
      // remove the system
    }
  }
}

void runSystems() {
  for (auto system : systems) {
    if (system.reads().size() == 0) {
      ready.push_back(system);
    }
    else {
      waitingOnWrites.push_back(system);
    }
  }
  runAllReady();
}
```

## Making registry thread safe

According to the [entt documentation](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system#multithreading), the registry is not thread safe.
This is not a huge problem as the system graph avoids multiple components being written to at the same time.
But the System type needs to be expanded to have `Creates<>` which takes all the types it creates.
Create types should be treated like writes, but not given as parameters to the update function, as the view cant find them yet.
`entt::registry::create` probably needs to happen with a lock.
Deletion of a entity should happen by adding a deletion tag which are then deleted by a single system later, as it modifies all components on an entity.

### Another way to thread safety

Possible using `Others<...>` (and maybe renaming it) could work for everything else the system access.
So if it creates and deletes entities, it be possible to just use registry as the type it access in Others.

## Parallel system

Each system could also be parallelized as they only operate on a single entity.
Then the ISystem could check if the class is a Parallel system if it is give it access to it's thread pool.
Through some scheduling is probably needed to make sure it does not starve other systems from starting.

```cpp

struct ParallelSystem<Self, size_t GroupSize, Reads<Read...>, Writes<Write...>, Others<Other...>> : virtual ISystem {
  void initParallel(thread_pool pool); 

  void update() // Get GroupSize entities and start a task with them in the thread pool. 
}
```

## Debug Physics view

The old system to show debugging of physics was too slow as it uploaded vertices each frame.
Instead it should be possible to save the shape in a component and draw each of them.
As most thing consists of simple geometry it should be possible to use instancing to draw these.
Mesh colliders and height maps might need to be uploaded and use the mesh handle explained earlier.
Moving the physics shapes could then be done with the transform.
<https://github.com/BoomingTech/Piccolo> has Jolt Physics debug view.

## New System base class

The old system of heavy templates seems cool, but it is harder to extend.
Merely having a base class which returns functions which are still type sets would be preferred i think.

## Using std::coroutine for async operations

std::coroutine have `co_await` which make it possible to suspend functions, allowing for possibly "cleaner" code.
An example of an use of this can be seen here: <https://www.reddit.com/r/cpp/comments/15n88a2/comment/jvlppb1/>
I am working on a generic coroutine library in the background to try and get something cool working which can support that syntax.
It should work something like this:

```cpp
task<Buffer> create_buffer_from(const void *data, size_t size, VkBufferUsageFlags usage)
{
    auto staging = create_staging_buffer_from(data, size);
    auto device = create_device_buffer(size, usage);
    auto command_buffer = begin_command_buffer();
    copy(command_buffer, staging, device, size);
    co_await command_buffer.execute_async();
    co_return device;
}

void Renderer::uploadMeshes(entt::registry& registry) {
  TaskList tasks{};
  auto task_generator = [](std::shared_ptr<AwaitingMesh> mesh, auto meshId) -> Task {
      auto uploadedVertexBuffer = co_await create_buffer_from(mesh->_vertices, ..., vk::BufferUsageFlagBits::eVertexBuffer);
      auto uploadedIndexBuffer = co_await create_buffer_from(mesh->_indices, ..., vk::BufferUsageFlagBits::eIndexBuffer);
      createMesh(meshId, uploadedVertexBuffer, uploadedIndexBuffer); // Sets the status
    };
  for (const auto entity : entities) {
    auto mesh = registry.remove<AwaitingMesh>(entity);
    auto meshId = getMeshId(mesh.path);
    registry.add<Mesh>(entity, {meshId});
    if (getStatus(meshID) != UPLOADING && != UPLOADED) // needs to be synchronized somehow
      tasks.add_task(task_generator(mesh.ptr, meshId));
  }
  scheduler->schedule_tasks(tasks); // Are executed at some point, no guarentee for when.
  // While a meshId is getting uploaded it cant be used for rendering. So when it is ready pop it in.
}
```

This really just needs a way co_await fences to be possible.

### Need to keep this in mind through

Host Synchronization:
Host access to queue must be externally synchronized
Host access to fence must be externally synchronized
-<https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html>

So need some kind of barrier there, but there can be quite a lot of queues, so spreading it out might be a good idea.
The GTX 1080ti has a total of 26 queues over three families (16, 2, 8) which would fit for transfer.
The queue family with only 2 queues is the transfer only family and might be preferred as it should be faster.
A counting semaphore and ringbuffer might be a good idea, or something similar.

## After discussion with vulkan Discord

This might seem fun, but also wildly complicated and probably unnecessary.
A single thread living in the background and waking to upload via a single transfer queue is much simpler and still allows for rendering to continue.
The above idea with IDs would probably still fit well as it would allow for just taking in a string and mapping it to an ID, which does not need any sync.
A map of ids to uploaded meshes would then be kept internally and ignore the ones not ready yet.
If two maps are kept as storage for new meshes uploaded, sync would also only have to be once per frame when the maps are swapped.
By keeping uploads on a single thread it should also simplify batching a lot of uploads together in a single command buffer, before submitting it.
Overall this would require much less work, be less prone to hard to diagnose problems and probably fast enough as the streaming requirements of my engine is likely to be low.

Using a [collector](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system#they-call-me-reactive-system) could also work for updating what should be kept in memory on the gpu. As it simplifies updates.
