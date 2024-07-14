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
#include <ChronosESP32.h>
#include <Preferences.h>
#include <Timber.h>
#include "FS.h"

#include "FFat.h"

#include <ArduinoJson.h>

#include "custom_face.h"
// #include "faces/34_2_dial/34_2_dial.h"
// #include "faces/75_2_dial/75_2_dial.h"
// #include "faces/79_2_dial/79_2_dial.h"
#include "faces/116_2_dial/116_2_dial.h"
#include "faces/756_2_dial/756_2_dial.h"
// #include "faces/b_w_resized/b_w_resized.h"
// #include "faces/kenya/kenya.h"
// #include "faces/pixel_resized/pixel_resized.h"
// #include "faces/radar/radar.h"
// #include "faces/smart_resized/smart_resized.h"
// #include "faces/tix_resized/tix_resized.h"
// #include "faces/wfb_resized/wfb_resized.h"

#include "main.h"

#define buf_size 10
#define MAX_FACES 15

#define FLASH FFat
#define F_NAME "FATFS"

class LGFX : public lgfx::LGFX_Device
{

  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Touch_CST816S _touch_instance;

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

    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;        // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max = 240;      // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min = 0;        // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max = 240;      // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int = TP_INT; // INTが接続されているピン番号
      // cfg.pin_rst = TP_RST;
      cfg.bus_shared = false;  // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 0; // 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定
      cfg.i2c_port = 0;        // 使用するI2Cを選択 (0 or 1)
      cfg.i2c_addr = 0x15;     // I2Cデバイスアドレス番号
      cfg.pin_sda = I2C_SDA;   // SDAが接続されているピン番号
      cfg.pin_scl = I2C_SCL;   // SCLが接続されているピン番号
      cfg.freq = 400000;       // I2Cクロックを設定

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // タッチスクリーンをパネルにセットします。
    }

    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

LGFX tft;

ChronosESP32 watch("Chronos Watchface");
Preferences prefs;

static const uint32_t screenWidth = 240;
static const uint32_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[2][screenWidth * buf_size];

typedef struct
{
  lv_obj_t **objs; // Array of lv_obj_t* pointers
  size_t count;    // Number of objects in the array
} lv_obj_array_t;

lv_obj_t *ui_faceSelect;
lv_obj_t *ui_home;

lv_obj_t *face_custom_root;

// SCREEN: ui_transferScreen
lv_obj_t *ui_transferScreen;
lv_obj_t *ui_fileInfoLabel;
lv_obj_t *ui_fileProgressBar;
lv_obj_t *ui_trnsferIcon;

int numFaces = 0;
int currentFace = 0;

bool formatRq = false;

struct Face
{
  const char *name;            // watchface name
  const lv_img_dsc_t *preview; // watchface preview image
  lv_obj_t **watchface;        // watchface root object pointer
  String path = "";
};

Face faces[MAX_FACES];

void update_faces();
bool load_custom_face(String file);
void check_local();
void registerWatchface_cb(const char *name, const lv_img_dsc_t *preview, lv_obj_t **watchface);
void register_custom(const char *name, const lv_img_dsc_t *preview, lv_obj_t **watchface, String path);

String hexString(uint8_t *arr, size_t len, bool caps = false, String separator = "");

bool readDialBytes(const char *path, uint8_t *data, size_t offset, size_t size);
bool isKnown(uint8_t id);
void parseDial(const char *path);
bool lv_img_header(uint8_t *byteArray, uint8_t cf, uint16_t w, uint16_t h);

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

  touched = tft.getTouch(&touchX, &touchY);

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

void *sd_open_cb(struct _lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
  char buf[256];
  sprintf(buf, "/%s", path);
  // Serial.print("path : ");
  // Serial.println(buf);

  File f;

  if (mode == LV_FS_MODE_WR)
  {
    f = FLASH.open(buf, FILE_WRITE);
  }
  else if (mode == LV_FS_MODE_RD)
  {
    f = FLASH.open(buf);
  }
  else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
  {
    f = FLASH.open(buf, FILE_WRITE);
  }

  if (!f)
  {
    return NULL; // Return NULL if the file failed to open
  }

  File *fp = new File(f); // Allocate the File object on the heap
  return (void *)fp;      // Return the pointer to the allocated File object
}

lv_fs_res_t sd_read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
  lv_fs_res_t res = LV_FS_RES_NOT_IMP;
  File *fp = (File *)file_p;
  uint8_t *buffer = (uint8_t *)buf;

  // Serial.print("name sd_read_cb : ");
  // Serial.println(fp->name());
  *br = fp->read(buffer, btr);

  res = LV_FS_RES_OK;
  return res;
}

lv_fs_res_t sd_seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
  lv_fs_res_t res = LV_FS_RES_OK;
  File *fp = (File *)file_p;

  uint32_t actual_pos;

  switch (whence)
  {
  case LV_FS_SEEK_SET:
    actual_pos = pos;
    break;
  case LV_FS_SEEK_CUR:
    actual_pos = fp->position() + pos;
    break;
  case LV_FS_SEEK_END:
    actual_pos = fp->size() + pos;
    break;
  default:
    return LV_FS_RES_INV_PARAM; // Invalid parameter
  }

  if (!fp->seek(actual_pos))
  {
    return LV_FS_RES_UNKNOWN; // Seek failed
  }

  // Serial.print("name sd_seek_cb : ");
  // Serial.println(fp->name());

  return res;
}

lv_fs_res_t sd_tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
  lv_fs_res_t res = LV_FS_RES_NOT_IMP;
  File *fp = (File *)file_p;

  *pos_p = fp->position();
  // Serial.print("name in sd_tell_cb : ");
  // Serial.println(fp->name());
  res = LV_FS_RES_OK;
  return res;
}

lv_fs_res_t sd_close_cb(struct _lv_fs_drv_t *drv, void *file_p)
{
  lv_fs_res_t res = LV_FS_RES_NOT_IMP;
  File *fp = (File *)file_p;

  // Serial.println("close");
  fp->close();
  res = LV_FS_RES_OK;
  return res;
}

void check_local()
{

  File root = FLASH.open("/");
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
    }
    else
    {
      // addListFile(file.name(), file.size());
      String nm = String(file.name());
      if (nm.endsWith(".jsn"))
      {
        // load watchface elements
        register_custom(nm.c_str(), &custom_preview, &face_custom_root, "/" + nm);
      }
      // if (nm.endsWith(".cbn"))
      // {
      //   // load watchface elements
      //   register_custom(nm.c_str(), &custom_preview, &face_custom_root, "/" + nm);
      // }
    }
    file = root.openNextFile();
  }
}

String readFile(const char *path)
{
  String result;
  File file = FLASH.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return result;
  }

  Serial.println("- read from file:");
  while (file.available())
  {
    result += (char)file.read();
  }
  file.close();
  return result;
}

void deleteFile(const char *path)
{
  Serial.printf("Deleting file: %s\r\n", path);
  if (FLASH.remove(path))
  {
    Serial.println("- file deleted");
  }
  else
  {
    Serial.println("- delete failed");
  }
}

void setup_fs()
{

  if (!FLASH.begin(true, "/ffat", 50))
  {
    Serial.println("FLASH Mount Failed");
    return;
  }

  Serial.print("Used bytes: ");
  Serial.println(FLASH.usedBytes());
  Serial.print("Available bytes: ");
  Serial.println(FLASH.totalBytes() - FLASH.usedBytes());
  Serial.print("Total bytes: ");
  Serial.println(FLASH.totalBytes());

  static lv_fs_drv_t sd_drv;
  lv_fs_drv_init(&sd_drv);
  sd_drv.cache_size = 512;

  sd_drv.letter = 'S';
  sd_drv.open_cb = sd_open_cb;
  sd_drv.close_cb = sd_close_cb;
  sd_drv.read_cb = sd_read_cb;
  sd_drv.seek_cb = sd_seek_cb;
  sd_drv.tell_cb = sd_tell_cb;
  lv_fs_drv_register(&sd_drv);

  check_local();
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
    if (currentFace != index)
    {
      currentFace = index;
      if (faces[index].path != "")
      {
        if (load_custom_face(faces[index].path))
        {
          ui_home = *faces[index].watchface;
        }
        else
        {
        }

        // parseDial(faces[index].path.c_str());
      }
      else
      {
        ui_home = *faces[index].watchface;
      }
    }

    lv_scr_load_anim(ui_home, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);

    Serial.print("Face selected: ");
    Serial.println(index);
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

void ui_transferScreen_screen_init(void)
{
  ui_transferScreen = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_transferScreen, LV_OBJ_FLAG_SCROLLABLE); /// Flags
  lv_obj_set_style_bg_color(ui_transferScreen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_transferScreen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_fileInfoLabel = lv_label_create(ui_transferScreen);
  lv_obj_set_width(ui_fileInfoLabel, 150);
  lv_obj_set_height(ui_fileInfoLabel, LV_SIZE_CONTENT); /// 1
  lv_obj_set_x(ui_fileInfoLabel, 0);
  lv_obj_set_y(ui_fileInfoLabel, 100);
  lv_obj_set_align(ui_fileInfoLabel, LV_ALIGN_TOP_MID);
  lv_label_set_text(ui_fileInfoLabel, "");
  lv_obj_set_style_text_font(ui_fileInfoLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_fileProgressBar = lv_bar_create(ui_transferScreen);
  lv_bar_set_value(ui_fileProgressBar, 0, LV_ANIM_OFF);
  lv_bar_set_start_value(ui_fileProgressBar, 0, LV_ANIM_OFF);
  lv_obj_set_width(ui_fileProgressBar, 150);
  lv_obj_set_height(ui_fileProgressBar, 10);
  lv_obj_set_x(ui_fileProgressBar, 0);
  lv_obj_set_y(ui_fileProgressBar, 70);
  lv_obj_set_align(ui_fileProgressBar, LV_ALIGN_TOP_MID);

  // ui_trnsferIcon = lv_img_create(ui_transferScreen);
  // lv_img_set_src(ui_trnsferIcon, &ui_img_file_download_png);
  // lv_obj_set_width(ui_trnsferIcon, LV_SIZE_CONTENT);   /// 1
  // lv_obj_set_height(ui_trnsferIcon, LV_SIZE_CONTENT);    /// 1
  // lv_obj_set_x(ui_trnsferIcon, 0);
  // lv_obj_set_y(ui_trnsferIcon, 20);
  // lv_obj_set_align(ui_trnsferIcon, LV_ALIGN_TOP_MID);
  // lv_obj_add_flag(ui_trnsferIcon, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
  // lv_obj_clear_flag(ui_trnsferIcon, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
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

void init_custom_face()
{
  face_custom_root = lv_obj_create(NULL);
  lv_obj_clear_flag(face_custom_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(face_custom_root, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(face_custom_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(face_custom_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(face_custom_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(face_custom_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(face_custom_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(face_custom_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(face_custom_root, onFaceEvent, LV_EVENT_ALL, NULL);
}

bool load_custom_face(String file)
{
  String read = readFile(file.c_str());
  JsonDocument face;
  DeserializationError err = deserializeJson(face, read);
  if (!err)
  {
    if (!face.containsKey("elements"))
    {
      return false;
    }
    String name = face["name"].as<String>();
    JsonArray elements = face["elements"].as<JsonArray>();
    int sz = elements.size();

    Serial.print(sz);
    Serial.println(" elements");

    invalidate_all();
    lv_obj_clean(face_custom_root);

    for (int i = 0; i < sz; i++)
    {
      JsonObject element = elements[i];
      int id = element["id"].as<int>();
      int x = element["x"].as<int>();
      int y = element["y"].as<int>();
      int pvX = element["pvX"].as<int>();
      int pvY = element["pvY"].as<int>();
      String image = element["image"].as<String>();
      JsonArray group = element["group"].as<JsonArray>();

      const char *group_arr[20];
      int group_size = group.size();
      for (int j = 0; j < group_size && j < 20; j++)
      {
        group_arr[j] = group[j].as<const char *>();
      }

      add_item(face_custom_root, id, x, y, pvX, pvY, image.c_str(), group_arr, group_size);
    }

    return true;
  }
  else
  {
    Serial.println("Deserialize failed");
  }

  return false;
}

void register_custom(const char *name, const lv_img_dsc_t *preview, lv_obj_t **watchface, String path)
{
  if (numFaces >= MAX_FACES)
  {
    return;
  }
  faces[numFaces].name = name;
  faces[numFaces].preview = preview;
  faces[numFaces].watchface = watchface;
  faces[numFaces].path = path;
  addWatchface(faces[numFaces].name, faces[numFaces].preview, numFaces);

  Timber.i("Custom Watchface: %s registered at %d", name, numFaces);
  numFaces++;
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

void configCallback(Config config, uint32_t a, uint32_t b)
{
  switch (config)
  {
  case CF_RST:

    formatRq = true;

    break;
  }
}

int cSize, pos, recv;
uint32_t total, currentRecv;
bool last;

String fName;

uint8_t buf1[1024];
uint8_t buf2[1024];
static bool writeFile = false, transfer = false, wSwitch = true;
static int wLen1 = 0, wLen2 = 0;
bool start = false;

void rawDataCallback(uint8_t *data, int len)
{
  if (data[0] == 0xB0)
  {
    // this is a chunk header data command
    cSize = data[1] * 256 + data[2];                                                           // data chunk size
    pos = data[3] * 256 + data[4];                                                             // position of the chunk, ideally sequential 0..
    last = data[7] == 1;                                                                       // whether this is the last chunk (1) or not (0)
    total = (data[8] * 256 * 256 * 256) + (data[9] * 256 * 256) + (data[10] * 256) + data[11]; // total size of the whole file
    recv = 0;                                                                                  // counter for the chunk data

    start = pos == 0;
    if (pos == 0)
    {
      // this is the first chunk
      transfer = true;
      currentRecv = 0;

      // lv_scr_load_anim(ui_transferScreen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
      // lv_label_set_text(ui_fileInfoLabel, "Receiving watchface file");

      fName = "/" + String(total, HEX) + "-" + String(total) + ".cbn";
    }
  }
  if (data[0] == 0xAF)
  {
    // this is the chunk data, line by line. The complete chunk will have several of these
    // actual data starts from index 5
    int ln = ((data[1] * 256 + data[2]) - 5); // byte 1 and 2 make up the (total size of data - 5)

    if (wSwitch)
    {
      memcpy(buf1 + recv, data + 5, ln);
    }
    else
    {
      memcpy(buf2 + recv, data + 5, ln);
    }

    recv += ln; // increment the received chunk data size by current received size

    currentRecv += ln; // track the progress

    if (recv == cSize)
    { // received expected? if data chunk size equals chunk receive size then chunk is complete
      if (wSwitch)
      {
        wLen1 = cSize;
      }
      else
      {
        wLen2 = cSize;
      }

      wSwitch = !wSwitch;
      writeFile = true;

      pos++;
      uint8_t lst = last ? 0x01 : 0x00;
      uint8_t cmd[5] = {0xB0, 0x02, highByte(pos), lowByte(pos), lst};
      watch.sendCommand(cmd, 5); // notify the app that we received the chunk, this will trigger transfer of next chunk
    }

    if (last)
    {

      // lv_label_set_text(ui_fileInfoLabel, "Transfer complete, parsing watchface");
    }
  }
}

void logCallback(Level level, unsigned long time, String message)
{
  Serial.print(message);
}

void my_log_cb(const char *buf)
{
  Serial.write(buf, strlen(buf));
}

void loadSplash()
{
  int w = 122;
  int h = 130;
  int xOffset = 63;
  int yOffset = 55;
  tft.setBrightness(200);
  tft.fillScreen(TFT_BLACK);
  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      tft.writePixel(x + xOffset, y + yOffset, uint16_t(splash[(((y * 122) + x) * 2)] << 8 | splash[(((y * 122) + x) * 2) + 1]));
    }
  }
  delay(2000);
}

void setup()
{
  Serial.begin(115200); /* prepare for possible serial debug */

  Timber.setLogCallback(logCallback);

  Timber.i("Starting up device");
  prefs.begin("my-app");

  tft.init();
  tft.initDMA();
  tft.startWrite();

  loadSplash();

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

  lv_log_register_print_cb(my_log_cb);

  _lv_fs_init();

  ui_home = lv_obj_create(NULL);

  ui_transferScreen_screen_init();

  init_face_select();
  init_custom_face();

  // init_face_34_2_dial(registerWatchface_cb);
  // init_face_75_2_dial(registerWatchface_cb);
  // init_face_79_2_dial(registerWatchface_cb);
  init_face_116_2_dial(registerWatchface_cb);
  init_face_756_2_dial(registerWatchface_cb);
  // init_face_b_w_resized(registerWatchface_cb);
  // init_face_kenya(registerWatchface_cb);
  // init_face_pixel_resized(registerWatchface_cb);
  // init_face_radar(registerWatchface_cb);
  // init_face_smart_resized(registerWatchface_cb);
  // init_face_tix_resized(registerWatchface_cb);
  // init_face_wfb_resized(registerWatchface_cb);

  setup_fs();

  if (numFaces == 0)
  {
    lv_obj_t *label1 = lv_label_create(ui_home);
    lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 100);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label1, screenWidth - 30);
    lv_label_set_text(label1, "No watchfaces detected. Check that they are enabled");

    lv_obj_t *slider1 = lv_slider_create(ui_home);
    lv_obj_set_width(slider1, screenWidth - 40);
    lv_obj_align_to(slider1, label1, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
  }
  else
  {
    String custom = prefs.getString("custom", "");
    if (custom != "" && load_custom_face(custom))
    {
      ui_home = face_custom_root;
    }
    else
    {
      ui_home = *faces[0].watchface;
    }
  }

  lv_disp_load_scr(ui_home);

  // watch.setConnectionCallback(connectionCallback);
  // watch.setNotificationCallback(notificationCallback);
  watch.setConfigurationCallback(configCallback);
  watch.setRawDataCallback(rawDataCallback);
  watch.begin();
  watch.set24Hour(true);
  watch.setBattery(70);

  tft.setBrightness(200);
}

void loop()
{
  watch.loop();
  if (!transfer)
  {

    lv_timer_handler(); /* let the GUI do its work */
    delay(5);

    update_faces();
  }

  if (formatRq)
  {
    FLASH.format(true);
    delay(2000);

    ESP.restart();
  }

  if (writeFile && transfer)
  {
    if (start)
    {
      tft.fillScreen(TFT_BLUE);

      tft.drawRoundRect(70, 120, 100, 20, 5, TFT_WHITE);
    }

    writeFile = false;

    File file = FLASH.open(fName, start ? FILE_WRITE : FILE_APPEND);
    if (file)
    {

      if (!wSwitch)
      {
        file.write(buf1, wLen1);
      }
      else
      {
        file.write(buf2, wLen2);
      }

      file.close();

      // Serial.print(last ? "Complete: " :  "");
      // Serial.print("Receieved ");
      // Serial.print(currentRecv);
      // Serial.print("/");
      // Serial.print(total);
      // Serial.print("   ");
      // Serial.println(hexString(cmd, 5, true, "-"));

      if (total > 0)
      {
        int progress = (100 * currentRecv) / total;

        // Serial.println(String(progress, 2) + "%");
        tft.setTextColor(TFT_WHITE, TFT_BLUE);
        tft.setTextSize(2);
        tft.setCursor(80, 80);
        tft.print(progress);
        tft.print("%");

        tft.fillRoundRect(70, 120, progress, 20, 5, TFT_WHITE);
      }

      if (last)
      {
        // the file transfer has ended
        transfer = false;

        tft.fillScreen(TFT_CYAN);
        tft.setTextColor(TFT_WHITE, TFT_CYAN);
        tft.setTextSize(2);
        tft.setCursor(80, 80);
        tft.print("Processing");

        parseDial(fName.c_str()); // process the file
      }
    }
    else
    {
      Serial.println("- failed to open file for writing");

      transfer = false;
    }
  }
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

  if (ui_home == face_custom_root)
  {
    update_time_custom(second, minute, hour, mode, am, day, month, year, weekday);
  }

  // update_check_34_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_75_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_79_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_116_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  update_check_756_2_dial(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_b_w_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_kenya(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_pixel_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_radar(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_smart_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_tix_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
  // update_check_wfb_resized(ui_home, second, minute, hour, mode, am, day, month, year, weekday, temp, icon, battery, connection, steps, distance, kcal, bpm, oxygen);
}

bool readDialBytes(const char *path, uint8_t *data, size_t offset, size_t size)
{
  File file = FLASH.open(path, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return false;
  }

  if (!file.seek(offset))
  {
    Serial.println("Failed to seek file");
    file.close();
    return false;
  }

  int bytesRead = file.readBytes((char *)data, size);

  if (bytesRead <= 0)
  {
    Serial.println("Error reading file");
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool isKnown(uint8_t id)
{
  if (id < 0x1E)
  {
    if (id != 0x04 || id != 0x05 || id != 0x12 || id != 0x18 || id != 0x20)
    {
      return true;
    }
  }
  else
  {
    if (id == 0xFA || id == 0xFD)
    {
      return true;
    }
  }
  return false;
}

String hexString(uint8_t *arr, size_t len, bool caps, String separator)
{
  String hexString = "";
  for (size_t i = 0; i < len; i++)
  {
    char hex[3];
    sprintf(hex, caps ? "%02X" : "%02x", arr[i]);
    hexString += separator;
    hexString += hex;
  }
  return hexString;
}

String longHexString(unsigned long l)
{
  char buffer[9];             // Assuming a 32-bit long, which requires 8 characters for hex representation and 1 for null terminator
  sprintf(buffer, "%08x", l); // Format as 8-digit hex with leading zeros
  return String(buffer);
}

void parseDial(const char *path)
{

  String name = longHexString(watch.getEpoch());

  Serial.print("Parsing dial:");
  Serial.println(path);

  JsonDocument json;
  JsonDocument elements;
  JsonDocument assetFiles;
  JsonArray elArray = elements.to<JsonArray>();
  JsonArray assetArray = assetFiles.to<JsonArray>();

  json["name"] = name;
  json["file"] = String(path);

  JsonDocument rsc;
  int errors = 0;

  uint8_t az[1];
  if (!readDialBytes(path, az, 0, 1))
  {
    Serial.println("Failed to read watchface header");
    errors++;
  }
  uint8_t j = az[0];

  static uint8_t item[20];
  static uint8_t table[512];

  uint8_t lid = 0;
  int a = 0;
  int lan = 0;
  int tp = 0;
  int wt = 0;

  for (int i = 0; i < j; i++)
  {
    if (i >= 60)
    {
      Serial.println("Too many watchface elements >= 60");
      break;
    }

    JsonDocument element;

    if (!readDialBytes(path, item, (i * 20) + 4, 20))
    {
      Serial.println("Failed to read element properties");
      errors++;
    }

    uint8_t id = item[0];

    element["id"] = id;

    uint16_t xOff = item[5] * 256 + item[4];
    uint16_t yOff = item[7] * 256 + item[6];

    element["x"] = xOff;
    element["y"] = yOff;

    uint16_t xSz = item[9] * 256 + item[8];
    uint16_t ySz = item[11] * 256 + item[10];

    uint32_t clt = item[15] * 256 * 256 * 258 + item[14] * 256 * 256 + item[13] * 256 + item[12];
    uint32_t dat = item[19] * 256 * 256 * 256 + item[18] * 256 * 256 + item[17] * 256 + item[16];

    uint8_t id2 = item[1];

    bool isG = (item[1] & 0x80) == 0x80;

    if (id == 0x08)
    {
      isG = true;
    }

    uint8_t cmp = isG ? (item[1] & 0x7F) : 1;

    int aOff = item[2];

    bool isM = (item[3] & 0x80) == 0x80;
    uint8_t cG = isM ? (item[3] & 0x7F) : 1;

    if (!isKnown(id))
    {
      continue;
    }

    if (id == 0x16 && (item[1] == 0x06 || item[1] == 0x00))
    {
      // weather (-) label
      continue;
    }
    if (isM)
    {
      lan++;
    }

    if (tp == 0x09 && id == 0x09)
    {
      a++;
    }
    else if (tp != id)
    {
      tp = id;
      a++;
    }
    else if (lan == 1)
    {
      a++;
    }

    if (xSz == 0 || ySz == 0)
    {
      continue;
    }

    int z = i;
    int rs = -1;

    bool createFile = false;

    if (rsc.containsKey(String(clt)))
    {
      z = rsc[String(clt)].as<int>();
      rs = z;
    }

    bool drawable = (id == 0x0d) ? (lan == 1 || lan == 17 || lan == 33) : true;

    JsonDocument grp;
    JsonArray grpArr = grp.to<JsonArray>();

    if (rs == -1 && drawable)
    {
      rsc[String(clt)] = i;

      for (int aa = 0; aa < cmp; aa++)
      {
        unsigned long nm = (i * 10000) + (clt * 10) + aa;
        grpArr.add("S:" + name + "_" + longHexString(nm) + ".bin");
      }

      if (id == 0x17)
      {
      }
      else if (id == 0x0A)
      {
      }
      else if (cmp == 1)
      {
      }
      else
      {
      }

      // save asset
      createFile = true;
    }
    else if (id == 0x16 && id2 == 0x00)
    {
      // save asset
      createFile = true;

      for (int aa = 0; aa < cmp; aa++)
      {
        unsigned long nm = (z * 10000) + (clt * 10) + aa;
        grpArr.add("S:" + name + "_" + longHexString(nm) + ".bin");
      }
    }
    else
    {

      for (int aa = 0; aa < cmp; aa++)
      {
        unsigned long nm = (z * 10000) + (clt * 10) + aa;
        grpArr.add("S:" + name + "_" + longHexString(nm) + ".bin");
      }
    }

    if (cmp <= 1)
    {
      // grp is null
      grpArr.clear();
    }

    if (id == 0x0A)
    {
      // if (connIC.count { it == '\n' } < 3) {
      //     continue
      // }
    }

    if (isM)
    {
      if (lan == cG)
      {
        lan = 0;
      }
      else if (id == 0x0d && (lan == 1 || lan == 32 || lan == 40 || lan == 17 || lan == 33))
      {
        yOff -= (ySz - aOff);
        xOff -= aOff;
      }
      else
      {
        continue;
      }
    }
    if (id == 0x17)
    {
      wt++;
      if (wt != 1)
      {
        continue;
      }
    }

    if (id == 0x16 && id2 == 0x06)
    {
      continue;
    }

    if (drawable)
    {
      element["pvX"] = aOff;
      element["pvY"] = ySz - aOff;

      unsigned long nm = (z * 10000) + (clt * 10) + 0;

      element["image"] = "S:" + name + "_" + longHexString(nm) + ".bin";
      element["group"] = grpArr;

      elArray.add(element);
    }

    Serial.printf("i:%d, id:%d, xOff:%d, yOff:%d, xSz:%d, ySz:%d, clt:%d, dat:%d, cmp:%d\n", i, id, xOff, yOff, xSz, ySz, clt, dat, cmp);

    if (!createFile)
    {
      continue;
    }
    uint8_t cf = (id == 0x09 && i == 0) || (id == 0x19) ? 0x04 : 0x05;
    bool tr = cf == 0x05;

    for (int b = 0; b < cmp; b++)
    {
      unsigned long nm = (z * 10000) + (clt * 10) + b;

      String asset = "/" + name + "_" + longHexString(nm) + ".bin";
      Serial.print("Create asset-> ");
      Serial.print(asset);

      assetArray.add(asset);

      uint8_t header[4];

      lv_img_header(header, cf, xSz, ySz / cmp);

      Serial.print("\t");
      Serial.println(hexString(header, 4));

      File ast = FLASH.open(asset.c_str(), FILE_WRITE);
      if (ast)
      {
        ast.write(header, 4);

        if (!readDialBytes(path, table, clt, 512))
        {
          Serial.println("Could not read color table bytes from file");
          errors++;
          break;
        }

        uint16_t yZ = uint16_t(ySz / cmp); // height of individual element

        File file = FLASH.open(path, "r");
        if (!file)
        {
          Serial.println("Failed to open file for reading");
          errors++;
          break;
        }
        int offset = (xSz * yZ) * b;

        if (!file.seek(dat + offset))
        {
          Serial.println("Failed to seek file");
          file.close();
          errors++;
          break;
        }

        int x = 0;
        if (id == 0x19)
        {
          for (int z = 0; z < (xSz * yZ); z++)
          {
            uint8_t pixel[2];
            pixel[0] = item[13];
            pixel[1] = item[12];
            ast.write(pixel, 2);
          }
        }
        else
        {
          while (file.available())
          {
            uint16_t index = file.read();

            uint8_t pixel[3];
            if (tr)
            {
              pixel[0] = table[(index * 2) + 1];
              pixel[1] = table[index * 2];
              pixel[2] = (uint16_t(pixel[0] * 256 + pixel[1]) == 0) ? 0x00 : 0xFF; // alpha byte (black pixel [0] is transparent)
              ast.write(pixel, 3);
            }
            else
            {
              pixel[0] = table[(index * 2) + 1];
              pixel[1] = table[index * 2];

              ast.write(pixel, 2);
            }
            x++;
            if (x >= (xSz * yZ))
            {
              break;
            }
          }
        }
        file.close();

        ast.close();
      }
      else
      {
        errors++;
      }
    }
  }

  json["elements"] = elements;
  json["assets"] = assetFiles;

  // serializeJsonPretty(json, Serial);

  String jsnFile = "/" + name + ".jsn";
  assetArray.add(jsnFile);
  File jsn = FLASH.open(jsnFile, FILE_WRITE);

  if (jsn)
  {
    serializeJsonPretty(json, jsn);
    jsn.flush();
    jsn.close();
  }
  else
  {
    errors++;
  }

  if (errors > 0)
  {
    // failed to parse watchface files
    // probably delete assetfiles
    Serial.print(errors);
    Serial.println(" errors encountered when parsing watchface");
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(80, 80);
    tft.print("Failed");
  }
  else
  {
    // success
    // probably delete source file

    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setTextSize(2);
    tft.setCursor(80, 80);
    tft.print("Success, rebooting");

    deleteFile(path);
    Serial.println("Watchface parsed successfully");
    // register_custom(name.c_str(), &custom_preview, &face_custom_root, jsnFile);

    prefs.putString("custom", jsnFile);

    ESP.restart();
  }
}

bool lv_img_header(uint8_t *byteArray, uint8_t cf, uint16_t w, uint16_t h)
{
  // Ensure the input values fit within the specified bit field sizes
  if (cf >= (1 << 5) || w >= (1 << 11) || h >= (1 << 11))
  {
    // Invalid input values
    return false;
  }

  uint32_t header = (cf & 0x1F) | (0 << 5) | (0 << 8) | ((w & 0x07FF) << 10) | ((h & 0x07FF) << 21);

  // Convert the 32-bit integer to bytes in little-endian format
  byteArray[0] = header & 0xFF;
  byteArray[1] = (header >> 8) & 0xFF;
  byteArray[2] = (header >> 16) & 0xFF;
  byteArray[3] = (header >> 24) & 0xFF;

  return true;
}
