// 针对 ECANNON 项目的 TFT_eSPI 配置
#define USER_SETUP_LOADED

// 定义显示屏驱动
#define ST7789_DRIVER

// 屏幕尺寸
#define TFT_WIDTH  480
#define TFT_HEIGHT 272

// ESP32-S3 的引脚配置
#define TFT_CS   10
#define TFT_DC    7
#define TFT_RST   6
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_MISO  13

// 触摸控制器
#define TOUCH_CS  16

// 启用 SPI 接口
#define TFT_SPI_PORT 1

// 颜色顺序
#define TFT_RGB_ORDER TFT_BGR

// 字体配置
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// SPI 频率
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
