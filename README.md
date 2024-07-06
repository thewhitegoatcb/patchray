# PatchRay
PatchRay is a collection of patches to the StingRay engine with various features.
# Patches
### lua_more_mem
Patch the lua memory allocator to have 2GB from 1GB
### hash_dump
Saves every murmur64 hashing usuage into `dict.txt`
# Installation
1. Install [ASI Loader x64](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/) into `binaries` or `binaries_dx12`
2. Copy the compiled [ASI plugins](https://github.com/thewhitegoatcb/patchray/releases) into the `binaries` or `binaries_dx12`
# Notes
- **The game will refuse to load with ASI Loader installed when EAC enabled (Official realm), you will have to remove or rename the `dinput8.dll` to something else temporarly**
- `hash_dump` will only be able to dump only when murmur64 hashing is used in runtime, some resources pre-compute the hash and store it to reference other resources, in this case there's no way to capture the original hash string in runtime