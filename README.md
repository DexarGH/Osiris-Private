# Osiris

[![Windows](https://github.com/danielkrupinski/Osiris/actions/workflows/windows.yml/badge.svg?branch=master&event=push)](https://github.com/danielkrupinski/Osiris/actions/workflows/windows.yml)
[![Linux](https://github.com/danielkrupinski/Osiris/actions/workflows/linux.yml/badge.svg?branch=master&event=push)](https://github.com/danielkrupinski/Osiris/actions/workflows/linux.yml)

Cross-platform (Windows, Linux) game hack for **Counter-Strike 2** with GUI and rendering based on game's Panorama UI. Compatible with the latest game update on Steam.

Since the developer Osiris is not very active right now, I decided to just add the features that I miss (and since my knowledge ends with Roblox, I did everything using Qwen Code :\ )

All rights belong to the original Osiris developer (https://github.com/danielkrupinski/osiris). My fork is terrible, and I can't argue with that (don't take this seriously).

## What's new
* 4 April 2026
    * Added World Particle (dont work, need fix)
    
* 3 April 2026
    * Change Aim Bot setting
    * Added FOV Circle
  
!!!Everything that was done below was NOT DONE BY ME!!!

* 13 March 2026
    * Added aimbot feature

* 04 November 2025
    * Improved smoothness of "Player Info in World" on moving players

* 30 October 2025
    * Added Bomb Plant Alert feature
        * Green color means the bomb will be planted before the end of the round if uninterrupted
        * Red color means the bomb can not be planted before the end of the round

    <img width="201" height="146" alt="Bomb Plant Alert" src="https://github.com/user-attachments/assets/21c0f8fb-a20d-42df-9857-f578cfc9b9f9" />

* 23 October 2025
    * Hostage Outline Glow hue is now customizable

* 20 October 2025
    * Added "No Scope Inaccuracy Visualization" feature

    <img height="300" alt="no scope inaccuracy visualization" src="https://github.com/user-attachments/assets/860c944a-00b1-4b67-9d41-6f43e46f4252" />

* 09 October 2025
    * Added viewmodel fov modification

    ![Viewmodel fov modification](https://github.com/user-attachments/assets/3b9d6bde-a68c-4739-913c-d3b6caba4117)

## Technical features

* C++ runtime library (CRT) is not used in release builds
* No heap memory allocations
* No static imports in release build on Windows
* No threads are created
* Exceptions are not used
* No external dependencies

## Compiling

### Prerequisites

#### Windows

* **Microsoft Visual Studio 2022** with **Desktop development with C++** workload

#### Linux

* **CMake 3.24** or newer
* **g++ 11 or newer** or **clang++ 18 or newer**

### Compiling from source

#### Windows

Open **Osiris.sln** in Visual Studio 2022, set build configuration to **Release | x64**. Press *Build solution* and you should receive **Osiris.dll** file.

#### Linux

**run build.sh**

After following these steps you should receive **libOsiris.so** file in **build/Source/** directory.

### Loading / Injecting into game process

#### Windows

You need a **DLL injector** to inject (load) **Osiris.dll** into game process.

Counter-Strike 2 blocks LoadLibrary injection method, so you have to use a manual mapping (aka reflective DLL injection) injector.

**Xenos** and **Extreme Injector** are known to be **detected** by VAC.

#### Linux

You can simply run **inject.sh** which is located in **build/Source/inject.sh**

However, this injection method might be detected by VAC as gdb is visible under **TracerPid** in `/proc/$(pidof cs2)/status` for the duration of the injection.

## FAQ

### Where are the settings stored on disk?

In a configuration file `default.cfg` inside `%appdata%\OsirisCS2\configs` directory on Windows and `$HOME/OsirisCS2/configs` on Linux.

## License

> Copyright (c) 2018-2025 Daniel Krupiński

This project is licensed under the [MIT License](https://opensource.org/licenses/mit-license.php) - see the [LICENSE](https://github.com/danielkrupinski/Osiris/blob/master/LICENSE) file for details.
