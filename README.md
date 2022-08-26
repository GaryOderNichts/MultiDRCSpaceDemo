# MultiDRCSpaceDemo
A space shooter demo using the Wii U's MultiDRC mode.  

The Wii U has an unused feature allowing two GamePads to be connected to a single console.  
Nintendo [announced this back at the E3 2012](https://www.polygon.com/gaming/2012/6/5/3065588/wii-u-supports-two-gamepads-nintendo-confirms) before the Wii U was released.  
While this feature was never released it is still in the system files and can be used by homebrew applications.  
This project acts as a small demo and example for using that MultiDRC mode. 

## Screenshots
![Screenshot 0](screenshot0.png)
![Screenshot 1](screenshot1.png)
![Screenshot 2](screenshot2.png)

## Controls
![GamePad controls](assets/controls.png)

## Assets
### Seamless Space Backgrounds - Screaming Brain Studios
[<img src="assets/background.png" width="128"> <img src="assets/border.png" width="128">](https://opengameart.org/content/seamless-space-backgrounds)

[![CC0-1.0](https://licensebuttons.net/l/zero/1.0/80x15.png)](http://creativecommons.org/publicdomain/zero/1.0/)

### Pixel Spaceship - dsonyy
[<img src="assets/spaceship_small_red.png" width="128"> <img src="assets/spaceship_small_blue.png" width="128">](https://opengameart.org/content/pixel-spaceship)

[![CC0-1.0](https://licensebuttons.net/l/zero/1.0/80x15.png)](http://creativecommons.org/publicdomain/zero/1.0/)

### Wii U controller illustration - Tokyoship
[<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/e/e3/Wii_U_controller_illustration.svg/320px-Wii_U_controller_illustration.svg.png" width="128">](https://commons.wikimedia.org/wiki/File:Wii_U_controller_illustration.svg)

[![CC-BY-3.0](https://licensebuttons.net/l/by/3.0/80x15.png)](https://creativecommons.org/licenses/by/3.0/)

## Building
To build this application you need devkitPPC and the dependencies below.  
Get started with installing the toolchain [here](https://devkitpro.org/wiki/Getting_Started).  
Then run `make`.

### Dependencies
- [wut](https://github.com/devkitPro/wut)  
  Note that the wut releases are currently missing PR [#263](https://github.com/devkitPro/wut/pull/263), [#264](https://github.com/devkitPro/wut/pull/264), [#265](https://github.com/devkitPro/wut/pull/265), [#266](https://github.com/devkitPro/wut/pull/266) which are required to build this project.
- ppc-glm
- ppc-libpng
- ppc-freetype

To install the dependencies run `(dkp-)pacman -S wut ppc-glm ppc-libpng ppc-freetype`


