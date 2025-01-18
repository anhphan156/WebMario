# Web Mario
A web game made using C & raylib. Click [here](https://anhphan156.github.io/WebMario/) to try.

## Build Instructions

### Linux Native App

Building this linux app requires gcc & raylib. For NixOS, run `nix develop` to have access to raylib. For other distros, you should refer to raylib's documentation for installation.

Then, `make && make run`

### Web

Building this requires emscripten installed. For NixOS, just hit `nix develop`. For others, please refer to enscripten website.

Also, raylib needs to be recompiled for web assembly support.

Then, `make web && emrun game`
