## Shader Animation  
Explore various shader animations.

## Quick Start  
To compile and run:

```bash
cc -o nob nob.c # Run this once to compile
./nob           # Start the build system
```

To run a shader:

```bash
# Usage: ./build/main <dynamic_library>
./build/main ./build/libexample.so
```

### Key Bindings
* <kbd>Q</kbd> — Exit the application
* <kbd>H</kbd> — Reload the shader (hot-reload)
* <kbd>B</kbd> — Restart shader animation (“Begin Again the Shader”)
* <kbd>S</kbd> — Take a screenshot of the current window
* <kbd>C</kbd> — Capture a high-quality frame from the render window
* <kbd>R</kbd> — Begin video rendering
* <kbd>Esc</kbd> — Stop rendering mode and return to normal view


### References  
- Inspired by examples from [ShaderToy](https://www.shadertoy.com/)
- [Palettes - Shader](https://iquilezles.org/articles/palettes/)
