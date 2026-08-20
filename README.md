# MCPE 0.8.1 (PS3 Edition)
An attempt to decompile MCPE 0.8.1 and to make it possible to run it natively on **PlayStation 3 (PS3)**. The project is not affiliated with Mojang or Microsoft.

## Building

### Cloning the repo and preparing assets
* Clone the repo with the `--recursive` flag (or run `git submodule init` and `git submodule update` after cloning if you forgot to add it).
* **⚠️ IMPORTANT: Place the original MCPE 0.8.1 `.apk` file directly in the root directory of the project.** You do not need to unzip it manually. The `Makefile` will automatically extract all the necessary files from the APK and move them to the correct directories inside the `build/` folder to generate the `.pkg`. *(Note: The compiled game crash if you skip this step).*
* Run `python tools/get_sound_data.py <path/to/libminecraftpe.so>` - it should generate `pcm_data.c`.
* Move `pcm_data.c` to `./minecraftpe/impl/`.

### Compiling for PlayStation 3
The PS3 port is built using a custom Docker container. It includes the open-source PSL1GHT SDK, Python 2.7, and the NVIDIA Cg Toolkit (required for compiling `.vcg` and `.fcg` shaders). 

**1. Build the Docker environment:**
Ensure you have Docker installed. Place the provided `Dockerfile` and `toolchain.sh` in your toolchain directory and build the image:
```bash
docker build -t ps3dev-mcpe .
