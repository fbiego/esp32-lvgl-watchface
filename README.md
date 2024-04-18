# ZSWatch LVGL Watchfaces

## Table of Contents

- [ZSWatch LVGL Watchfaces](#zswatch-lvgl-watchfaces)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
  - [Video](#video)
  - [Watchfaces](#watchfaces)
      - [Preview](#preview)
  - [Preparation (Linux)](#preparation-linux)
  - [Usage](#usage)
  - [Maintainer](#maintainer)

## About

This project renders pre-built binary watchfaces on ESP32 using LVGL. A Kotlin script is used to transform the binary watchfaces into LVGL code, which is then compiled for the [ZSWatch](https://github.com/jakkra/ZSWatch).

Forked from [esp32-lvgl-watchface](https://github.com/fbiego/esp32-lvgl-watchface).

## Video

[`Watchfaces demo`](https://youtu.be/lvRsTp9v6_k)

## Watchfaces

Watchfaces can be obtained from [watch-face-wearfit](https://github.com/fbiego/watch-face-wearfit) or [Chronos Dials](https://chronos.ke/dials). Since the ZSWatch uses a 240x240 screen, watchfaces of the same resolution are recommended.

#### Preview

| | | |
| -- | -- | -- |
| !["Analog"](previews/75_2_dial/watchface.png "75_2_dial") | !["Shadow"](previews/34_2_dial/watchface.png "34_2_dial") | !["Blue"](previews/79_2_dial/watchface.png "79_2_dial") |
| !["Radar"](previews/radar/watchface.png "radar") | !["Outline"](previews/116_2_dial/watchface.png "116_2_dial") | !["Red"](previews/756_2_dial/watchface.png "756_2_dial") |
| !["Tix"](previews/tix_resized/watchface.png "tix_resized") | !["Pixel"](previews/pixel_resized/watchface.png "pixel_resized") | !["Smart"](previews/smart_resized/watchface.png "smart_resized") |
| !["Kenya"](previews/kenya/watchface.png "kenya") | !["B & W"](previews/b_w_resized/watchface.png "b_w_resized") | !["WFB"](previews/wfb_resized/watchface.png "wfb_resized") |

## Preparation (Linux)

- Install Kotlin by using `sudo apt-get install kotlin`

## Usage

Compile the script [`bin2lvgl.kt`](bin2lvgl.kt) into `bin2lvgl.jar` by using the following command:

```sh
kotlinc bin2lvgl.kt -include-runtime -d bin2lvgl.jar
```

Download a watchface via `wget`:

```sh
wget https://github.com/fbiego/watch-face-wearfit/raw/main/dials/X5Pro/1005.bin -O watchface.bin
```

To convert the watchface, simply run the following command:

```sh
java -jar bin2lvgl.jar watchface.bin
```

The LVGL code will be generated into a `output` folder and this folder must be copied into the `watchfaces` directory of the ZSWatch project:

```sh
cp <output>/* <ZSWatchPath>/ZSWatch/app/src/ui/watchfaces/<output>/
```

**TBD How to add it to the firmware**

!["LVGL watchface"](esp32_lvgl_watchface.png "watchface") 

## Maintainer

- [Daniel Kampert](mailto:daniel.kameprt@kampis-elektroecke.de)
