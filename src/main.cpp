/*
   MIT License

  Copyright (c) 2024 Felix Biego

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  ______________  _____
  ___  __/___  /_ ___(_)_____ _______ _______
  __  /_  __  __ \__  / _  _ \__  __ `/_  __ \
  _  __/  _  /_/ /_  /  /  __/_  /_/ / / /_/ /
  /_/     /_.___/ /_/   \___/ _\__, /  \____/
                              /____/

*/

#define LGFX_USE_V1
#include "Arduino.h"
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include "CST816D.h"
#include <ChronosESP32.h>
#include <Timber.h>



#include "faces/34_2_dial/34_2_dial.h"
#include "faces/75_2_dial/75_2_dial.h"
#include "faces/79_2_dial/79_2_dial.h"
#include "faces/116_2_dial/116_2_dial.h"
#include "faces/756_2_dial/756_2_dial.h"
#include "faces/b_w_resized/b_w_resized.h"
#include "faces/kenya/kenya.h"
#include "faces/pixel_resized/pixel_resized.h"
#include "faces/radar/radar.h"
#include "faces/smart_resized/smart_resized.h"
#include "faces/tix_resized/tix_resized.h"
#include "faces/wfb_resized/wfb_resized.h"

#include "main.h"

#define buf_size 10
#define MAX_FACES 15

class LGFX : public lgfx::LGFX_Device
{

  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();

      // SPIバスの設定
      cfg.spi_host = SPI; // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode = 0;                  // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 80000000;         // 传输时的SPI时钟（最高80MHz，四舍五入为80MHz除以整数得到的值）
      cfg.freq_read = 20000000;          // 接收时的SPI时钟
      cfg.spi_3wire = true;              // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock = true;               // 使用事务锁时设置为 true
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = SCLK; // SPIのSCLKピン番号を設定
      cfg.pin_mosi = MOSI; // SPIのCLKピン番号を設定
      cfg.pin_miso = MISO; // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc = DC;     // SPIのD/Cピン番号を設定  (-1 = disable)

      _bus_instance.config(cfg);              // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance); // バスをパネルにセットします。
    }

    {                                      // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs = CS;   // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst = RST; // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy = -1; // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定さ BUSYが接続されているピン番号 (-1 = disable)れていますので、不明な項目はコメントアウトして試してみてください。

      cfg.memory_width = 240;   // ドライバICがサポートしている最大の幅
      cfg.memory_height = 240;  // ドライバICがサポートしている最大の高さ
      cfg.panel_width = 240;    // 実際に表示可能な幅
      cfg.panel_height = 240;   // 実際に表示可能な高さ
      cfg.offset_x = 0;         // パネルのX方向オフセット量
      cfg.offset_y = 0;         // パネルのY方向オフセット量
      cfg.offset_rotation = 0;  // 值在旋转方向的偏移0~7（4~7是倒置的）
      cfg.dummy_read_pixel = 8; // 在读取像素之前读取的虚拟位数
      cfg.dummy_read_bits = 1;  // 读取像素以外的数据之前的虚拟读取位数
      cfg.readable = false;     // 如果可以读取数据，则设置为 true
      cfg.invert = true;        // 如果面板的明暗反转，则设置为 true
      cfg.rgb_order = false;    // 如果面板的红色和蓝色被交换，则设置为 true
      cfg.dlen_16bit = false;   // 对于以 16 位单位发送数据长度的面板，设置为 true
      cfg.bus_shared = false;   // 如果总线与 SD 卡共享，则设置为 true（使用 drawJpgFile 等执行总线控制）

      _panel_instance.config(cfg);
    }

    {                                      // Set backlight control. (delete if not necessary)
      auto cfg = _light_instance.config(); // Get the structure for backlight configuration.

      cfg.pin_bl = BL;     // pin number to which the backlight is connected
      cfg.invert = false;  // true to invert backlight brightness
      cfg.freq = 44100;    // backlight PWM frequency
      cfg.pwm_channel = 1; // PWM channel number to use

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance); // Sets the backlight to the panel.
    }

    setPanel(&_panel_instance); // 使用するパネルをセットします。
                                //    { // バックライト制御の設定を行います。(必要なければ削除）
                                //    auto cfg = _light_instance.config();// バックライト設定用の構造体を取得します。
                                //    cfg.pin_bl = 8;             // バックライトが接続されているピン番号 BL
                                //    cfg.invert = false;          // バックライトの輝度を反転させる場合 true
                                //    cfg.freq   = 44100;          // バックライトのPWM周波数
                                //    cfg.pwm_channel = 7;         // 使用するPWMのチャンネル番号
                                //    _light_instance.config(cfg);
                                //    _panel_instance.setLight(&_light_instance);//バックライトをパネルにセットします。
                                //    }
  }
};

LGFX tft;
CST816D touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);
ChronosESP32 watch("Chronos Watchface");

static const uint32_t screenWidth = 240;
static const uint32_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[2][screenWidth * buf_size];

lv_obj_t *ui_faceSelect;
lv_obj_t *ui_home;

int numFaces = 0;

struct Face
{
  const char *name;            // watchface name
  const lv_img_dsc_t *preview; // watchface preview image
  lv_obj_t **watchface;        // watchface root object pointer
};

Face faces[MAX_FACES];

void update_faces();

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  if (tft.getStartCount() == 0)
  {
    tft.endWrite();
  }

  tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

  bool touched;
  uint8_t gesture;
  uint16_t touchX, touchY;

  touched = touch.getTouch(&touchX, &touchY, &gesture);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

void onFaceSelected(lv_event_t *e)
{
  lv_event_code_t event_code = lv_event_get_code(e);
  lv_obj_t *target = lv_event_get_target(e);
  int index = (int)lv_event_get_user_data(e);

  if (event_code == LV_EVENT_CLICKED)
  {
    if (index >= numFaces)
    {
      return;
    }
    ui_home = *faces[index].watchface;
    lv_scr_load_anim(ui_home, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
  }
}

void onFaceEvent(lv_event_t *e)
{
  lv_event_code_t event_code = lv_event_get_code(e);
  lv_obj_t *target = lv_event_get_target(e);

  if (event_code == LV_EVENT_LONG_PRESSED)
  {
    lv_scr_load_anim(ui_faceSelect, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
  }
}

void addWatchface(const char *name, const lv_img_dsc_t *src, int index)
{

  lv_obj_t *ui_faceItem = lv_obj_create(ui_faceSelect);
  lv_obj_set_width(ui_faceItem, 160);
  lv_obj_set_height(ui_faceItem, 180);
  lv_obj_set_align(ui_faceItem, LV_ALIGN_CENTER);
  lv_obj_clear_flag(ui_faceItem, LV_OBJ_FLAG_SCROLLABLE); /// Flags
  lv_obj_set_style_radius(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_faceItem, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_faceItem, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_color(ui_faceItem, lv_color_hex(0x142ABC), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_opa(ui_faceItem, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(ui_faceItem, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_pad(ui_faceItem, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(ui_faceItem, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *ui_facePreview = lv_img_create(ui_faceItem);
  lv_img_set_src(ui_facePreview, src);
  lv_obj_set_width(ui_facePreview, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_height(ui_facePreview, LV_SIZE_CONTENT); /// 1
  lv_obj_set_align(ui_facePreview, LV_ALIGN_TOP_MID);
  lv_obj_add_flag(ui_facePreview, LV_OBJ_FLAG_ADV_HITTEST);  /// Flags
  lv_obj_clear_flag(ui_facePreview, LV_OBJ_FLAG_SCROLLABLE); /// Flags

  lv_obj_t *ui_faceLabel = lv_label_create(ui_faceItem);
  lv_obj_set_width(ui_faceLabel, 160);
  lv_obj_set_height(ui_faceLabel, LV_SIZE_CONTENT); /// 1
  lv_obj_set_align(ui_faceLabel, LV_ALIGN_BOTTOM_MID);
  lv_label_set_long_mode(ui_faceLabel, LV_LABEL_LONG_DOT);
  lv_label_set_text(ui_faceLabel, name);
  lv_obj_set_style_text_align(ui_faceLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_faceLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_faceItem, onFaceSelected, LV_EVENT_ALL, (void *)index);
}

void init_face_select()
{
  ui_faceSelect = lv_obj_create(NULL);
  lv_obj_set_width(ui_faceSelect, 240);
  lv_obj_set_height(ui_faceSelect, 240);
  lv_obj_set_align(ui_faceSelect, LV_ALIGN_CENTER);
  lv_obj_set_flex_flow(ui_faceSelect, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ui_faceSelect, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(ui_faceSelect, LV_OBJ_FLAG_SNAPPABLE); /// Flags
  lv_obj_set_scrollbar_mode(ui_faceSelect, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_radius(ui_faceSelect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_faceSelect, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_faceSelect, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ui_faceSelect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(ui_faceSelect, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(ui_faceSelect, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(ui_faceSelect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(ui_faceSelect, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(ui_faceSelect, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_column(ui_faceSelect, 15, LV_PART_MAIN | LV_STATE_DEFAULT);

}

void registerWatchface_cb(const char *name, const lv_img_dsc_t *preview, lv_obj_t **watchface)
{
  if (numFaces >= MAX_FACES)
  {
    return;
  }
  faces[numFaces].name = name;
  faces[numFaces].preview = preview;
  faces[numFaces].watchface = watchface;
  addWatchface(faces[numFaces].name, faces[numFaces].preview, numFaces);

  Timber.i("Watchface: %s registered at %d", name, numFaces);
  numFaces++;
}

void logCallback(Level level, unsigned long time, String message)
{
  Serial.print(message);
}

void setup()
{
  Serial.begin(115200); /* prepare for possible serial debug */

  Timber.setLogCallback(logCallback);

  Timber.i("Starting up device");

  tft.init();
  tft.initDMA();
  tft.startWrite();

  touch.begin();
  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf[0], buf[1], screenWidth * buf_size);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  lv_disp_t *dispp = lv_disp_get_default();
  lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  ui_home = lv_obj_create(NULL);

  init_face_select();

  init_face_34_2_dial(registerWatchface_cb);
  init_face_75_2_dial(registerWatchface_cb);
  init_face_79_2_dial(registerWatchface_cb);
  init_face_116_2_dial(registerWatchface_cb);
  init_face_756_2_dial(registerWatchface_cb);
  init_face_b_w_resized(registerWatchface_cb);
  init_face_kenya(registerWatchface_cb);
  init_face_pixel_resized(registerWatchface_cb);
  init_face_radar(registerWatchface_cb);
  init_face_smart_resized(registerWatchface_cb);
  init_face_tix_resized(registerWatchface_cb);
  init_face_wfb_resized(registerWatchface_cb);

  

  if (numFaces == 0){
    lv_obj_t *label1 = lv_label_create(ui_home);
    lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 100);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label1, screenWidth - 30);
    lv_label_set_text(label1, "No watchfaces detected. Check that they are enabled");

    lv_obj_t *slider1 = lv_slider_create(ui_home);
    lv_obj_set_width(slider1, screenWidth - 40);
    lv_obj_align_to(slider1, label1, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
  } else {
    ui_home = *faces[0].watchface;
  }

  lv_disp_load_scr(ui_home);

  // watch.setConnectionCallback(connectionCallback);
  // watch.setNotificationCallback(notificationCallback);
  // watch.setConfigurationCallback(configCallback);
  watch.begin();
  watch.set24Hour(true);
  watch.setBattery(70);

  tft.setBrightness(200);
}

void loop()
{

  lv_timer_handler(); /* let the GUI do its work */
  delay(5);

  watch.loop();

  update_faces();
}

void update_faces()
{
  int second = watch.getSecond();
  int minute = watch.getMinute();
  int hour = watch.getHourC();
  bool mode = watch.is24Hour();
  bool am = watch.getHour(true) < 12;
  int day = watch.getDay();
  int month = watch.getMonth() + 1;
  int year = watch.getYear();
  int weekday = watch.getDayofWeek();

  int temp = watch.getWeatherAt(0).temp;
  int icon = watch.getWeatherAt(0).icon;

  int battery = watch.getPhoneBattery();
  int connection = watch.isConnected();

  int steps = 2735;
  int distance = 17;
  int kcal = 348;
  int bpm = 76;
  int oxygen = 97;

  update_check_34_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_75_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_79_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_116_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_756_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_b_w_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_kenya(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_pixel_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_radar(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_smart_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_tix_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_wfb_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
}
