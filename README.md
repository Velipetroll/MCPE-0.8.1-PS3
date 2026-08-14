# MCPE 0.8.1 (PS3 Edition)
An attempt to decompile MCPE 0.8.1 and to make it possible to run it natively on **PlayStation 3 (PS3)**. The project is not affiliated with Mojang or Microsoft.

## Building

### Cloning the repo and extracting sounds
* Clone the repo with `--recursive` flag (or use `git submodule init` and `git submodule update` after cloning if you forgot to add it)
* You must have the original 0.8.1 apk file to extract sounds and obtain assets (the compiled file will crash or not work properly without them).
* Run `python tools/get_sound_data.py <path/to/libminecraftpe.so>` - it should generate `pcm_data.c`
* Move `pcm_data.c` to `./minecraftpe/impl/`

### Compiling for PlayStation 3
The PS3 port is built using a custom Docker container. It includes the open-source PSL1GHT SDK, Python 2.7, and the NVIDIA Cg Toolkit (required for compiling `.vcg` and `.fcg` shaders). 

**1. Build the Docker environment:**
Ensure you have Docker installed. Place the provided `Dockerfile` and `toolchain.sh` in your toolchain directory and build the image:
```bash
docker build -t ps3dev-mcpe .
```

**2. Compile the game:**
Navigate to the root of the MCPE project (where the `Makefile` is located) and run the container to compile:
```bash
# On Linux / Mac / Git Bash:
docker run --rm -it -v "$PWD:/mcpe" -w /mcpe ps3dev-mcpe make clean all

# On Windows (CMD):
docker run --rm -it -v "%cd%:/mcpe" -w /mcpe ps3dev-mcpe make clean all
```
The output will be placed in the `build/` directory, containing the `.elf` executables and a ready-to-use PS3 Jailbreak Folder (`MCPE00801`).

## Running (PS3 / RPCS3)
* Extract the `assets` folder from the real MCPE 0.8.1 apk.
* Copy the `assets` folder inside the generated PS3 build directory: `build/MCPE00801/USRDIR/`
* Move the `MCPE00801` folder to `/dev_hdd0/game/` on a jailbroken PS3, or boot the folder directly using the RPCS3 Emulator.

## Some additional info:
* JSON library that was probably used by Mojang: https://chromium.googlesource.com/external/jsoncpp/+/6921bf1feef6f1fb83935ae3943f07753488311d/jsoncpp
* RakNet: https://web.archive.org/web/20260101222408if_/http://www.raknet.com/raknet/downloads/RakNet_PC-4.036.zip (might be some other version, probably modifed by mojang in 0.1.x)
* GZIP stuff - zlib 1.2.3, based on https://zlib.net/zpipe.c
* https://github.com/nothings/stb/
* GLM - commit before https://github.com/g-truc/glm/commit/2b747cbbadfd3af39b443e88902f1c98bd231083 and -DGLM_FORCE_RADIANS <?>
* OpenAES - used for realms stuff
