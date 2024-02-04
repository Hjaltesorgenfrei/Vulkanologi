# Rotte Engine

A WIP Game engine written in C++ 23 using Vulkan. Rotte is Danish for rat, and is the spirit animal of the engine.

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
["Low poly race track"](https://skfb.ly/ooYNR) by Quarks Studios is licensed under [Creative Commons Attribution](http://creativecommons.org/licenses/by/4.0/).

## Read More

See [Design](./DESIGN.md) for pondering I have done on how to design various parts of the engine.
See [TODO](./TODO.md) for what the current todos are.
