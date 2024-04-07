# ESP32 LVGL Watchfaces

This project renders pre-built binary watchfaces on ESP32 using LVGL. A Kotlin script is used to transform the binary watchfaces into LVGL code, which is then compiled for the ESP32 display.


## Video

[`Watchfaces demo`](https://youtu.be/lvRsTp9v6_k)

## Watchfaces

Watchfaces can be obtained from [watch-face-wearfit](https://github.com/fbiego/watch-face-wearfit) or [Chronos Dials](https://chronos.ke/dials). Since this project uses a 240x240 screen, watchfaces of the same resolution are recommended.

#### Preview

| | | |
| -- | -- | -- |
| !["Analog"](src/faces/75_2_dial/watchface.png?raw=true "75_2_dial") | !["Shadow"](src/faces/34_2_dial/watchface.png?raw=true "34_2_dial") | !["Blue"](src/faces/79_2_dial/watchface.png?raw=true "79_2_dial") |
| !["Radar"](src/faces/radar/watchface.png?raw=true "radar") | !["Outline"](src/faces/116_2_dial/watchface.png?raw=true "116_2_dial") | !["Red"](src/faces/756_2_dial/watchface.png?raw=true "756_2_dial") |
| !["Tix"](src/faces/tix_resized/watchface.png?raw=true "tix_resized") | !["Pixel"](src/faces/pixel_resized/watchface.png?raw=true "pixel_resized") | !["Smart"](src/faces/smart_resized/watchface.png?raw=true "smart_resized") |
| !["Kenya"](src/faces/kenya/watchface.png?raw=true "kenya") | !["B & W"](src/faces/b_w_resized/watchface.png?raw=true "b_w_resized") | !["WFB"](src/faces/wfb_resized/watchface.png?raw=true "wfb_resized") |

## Usage

The Kotlin script [`bin2lvgl.kt`](src/faces/bin2lvgl.kt) has been compiled into `bin2lvgl.jar`.

You can also recompile it with `kotlinc bin2lvgl.kt -include-runtime -d bin2lvgl.jar`.

To convert the watchface, simply run `java -jar bin2lvgl.jar watchface.bin`, and LVGL code will be generated into a folder.
On Windows, just drag and drop the bin file to [`convert.bat`](src/faces/convert.bat)

In the `main.cpp`, include the `watchface.h` file.

You will need to implement a callback for the watchface events:

```
void onFaceEvent(lv_event_t *e){

}
```
After initializing LVGL, initialize the watchface:
```
init_face_watchface();
lv_disp_load_scr(face_watchface); // load the watchface root object to display it
```

In the loop, use the functions to update the watchface:
```
update_time_watchface(sec, min, hr, md, am, dy, mt, yr, wk);
update_weather_watchface(temp, ic);
update_status_watchface(bat, con);
update_activity_watchface(2735, 357, 345);
update_health_watchface(76, 97);
```

This project implements using multiple watchfaces using a `face_selector` object. Note that the number of watchfaces you can compile is limited by the target ESP32 flash size.

This project only demonstrates watchfaces that are functional to display time by leveraging the [`ChronosESP32 library`](https://github.com/fbiego/chronos-esp32).

For other advanced features such as notifications and control, check out the [`esp32-c3-mini`](https://github.com/fbiego/esp32-c3-mini) and [`dt78-esp32-firmware`](https://github.com/fbiego/dt78-esp32-firmware) projects.

!["LVGL watchface"](esp32_lvgl_watchface.png?raw=true "watchface") 
