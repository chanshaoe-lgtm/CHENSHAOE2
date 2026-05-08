#define JPEGDEC_NO_FILE_FUNCTIONS
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <OneButton.h>
#include <WiFiUDP.h>
#include <JPEGDEC.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define UDP_PORT 8888
#define RESET_BUTTON 0
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_RECONNECT_ATTEMPTS 3
#define CONFIG_TIMEOUT 300000

// 触摸按键区域定义
#define TOUCH_KEY_A_X 120
#define TOUCH_KEY_A_Y 190
#define TOUCH_KEY_B_X 200
#define TOUCH_KEY_B_Y 190
#define TOUCH_KEY_C_X 280
#define TOUCH_KEY_C_Y 190
#define TOUCH_KEY_D_X 360
#define TOUCH_KEY_D_Y 190
#define TOUCH_KEY_LEFT_X 50
#define TOUCH_KEY_LEFT_Y 136
#define TOUCH_KEY_RIGHT_X 430
#define TOUCH_KEY_RIGHT_Y 136
#define TOUCH_KEY_SIZE 50

// 触摸事件类型
#define TOUCH_EVENT_NONE 0
#define TOUCH_EVENT_A 1
#define TOUCH_EVENT_B 2
#define TOUCH_EVENT_C 3
#define TOUCH_EVENT_D 4
#define TOUCH_EVENT_LEFT 5
#define TOUCH_EVENT_RIGHT 6

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);
OneButton resetButton(RESET_BUTTON, true, true);
WiFiUDP udp;
Preferences preferences;
JPEGDEC jpeg;

// 触摸状态变量
bool lastTouchState = false;
unsigned long lastTouchTime = 0;
int lastTouchEvent = TOUCH_EVENT_NONE;
unsigned long lastHeartbeatSend = 0;

// WiFi配置状态
enum ConfigState { STATE_SCAN, STATE_SELECT, STATE_PASSWORD, STATE_CONNECTING, STATE_SUCCESS };
ConfigState configState = STATE_SCAN;
int selectedWiFiIndex = 0;
int wifiCount = 0;
int wifiScrollOffset = 0;
String wifiSSID = "";
String wifiPassword = "";
String wifiPasswordInput = "";
bool showPassword = false;
int keyboardMode = 0;  // 0:小写 1:大写 2:数字 3:符号
int lastAlphaMode = 0; // 记录上次的字母模式

// 虚拟键盘
const char* keyboardLower = "qwertyuiopasdfghjklzxcvbnm";
const char* keyboardUpper = "QWERTYUIOPASDFGHJKLZXCVBNM";
const char* keyboardNumbers = "1234567890";
const char* keyboardSymbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";

// 配网模式枚举
enum DeviceMode { MODE_WAIT, MODE_CONFIG_TOUCH, MODE_CONFIG_AP, MODE_WORKING };
DeviceMode currentMode = MODE_WAIT;

bool hasTouch = false;
unsigned long configStartTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastFrameTime = 0;
int reconnectAttempts = 0;

const char* AP_SSID_BASE = "赛车仪表-";
IPAddress local_IP;
IPAddress remoteIP;
uint16_t remotePort = 0;
WiFiClient wifiClient;
bool firstFrameReceived = false;

// 函数声明
void drawTextCenter(const char* text, int y, uint32_t color = TFT_WHITE);
void drawStatusScreen(const char* status, const char* detail = "");
bool isInTouchArea(int touchX, int touchY, int keyX, int keyY, int size);
int detectTouchEvent();
void sendTouchEvent(int eventType);
void detectTouch();
void drawWiFiList();
void drawPasswordScreen();
void drawKeyboard();
bool handleKeyboardTouch(uint16_t touchX, uint16_t touchY);
void scanWiFi();
void connectToWiFi();
void handleConfigTouch();
bool connectToSavedWiFi();
void showIPAddress();
void saveWiFiConfig(const char* ssid, const char* password);
void clearWiFiConfig();
void startAPConfig();
void startTouchConfig();
void resetConfig();
int JPEGDraw(JPEGDRAW* pDraw);
void handleUDP();

void drawTextCenter(const char* text, int y, uint32_t color) {
    tft.setTextSize(2);
    tft.setTextColor(color);
    int16_t x = (SCREEN_WIDTH - tft.textWidth(text)) / 2;
    tft.setCursor(x, y);
    tft.print(text);
}

void drawStatusScreen(const char* status, const char* detail) {
    tft.fillScreen(TFT_BLACK);
    drawTextCenter("ECAN赛车无线屏", 50, TFT_RED);
    tft.setTextSize(1);
    drawTextCenter(status, 120, TFT_WHITE);
    if (strlen(detail) > 0) {
        drawTextCenter(detail, 155, TFT_SILVER);
    }
}

bool isInTouchArea(int touchX, int touchY, int keyX, int keyY, int size) {
    return (touchX >= keyX - size/2 && touchX <= keyX + size/2 &&
            touchY >= keyY - size/2 && touchY <= keyY + size/2);
}

int detectTouchEvent() {
    uint16_t touchX = 0, touchY = 0;
    
    // 简单的触摸检测 - 实际项目中需要根据具体硬件调整
    // 这里模拟触摸检测
    return TOUCH_EVENT_NONE;
}

void sendTouchEvent(int eventType) {
    if (remotePort == 0) return;
    
    uint8_t touchPacket[2];
    touchPacket[0] = 'T';  // 触摸事件标识
    touchPacket[1] = eventType;
    
    udp.beginPacket(remoteIP, remotePort);
    udp.write(touchPacket, sizeof(touchPacket));
    udp.endPacket();
}

void detectTouch() {
    pinMode(4, INPUT);
    hasTouch = (digitalRead(4) == HIGH);
}

void drawWiFiList() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor((SCREEN_WIDTH - tft.textWidth("选择WiFi")) / 2, 20);
    tft.print("选择WiFi");
    tft.setTextSize(1);
    
    // 计算显示范围
    int maxVisible = 4;
    int startIndex = wifiScrollOffset;
    int endIndex = min(wifiCount, startIndex + maxVisible);
    
    for (int i = startIndex; i < endIndex; i++) {
        int y = 55 + (i - startIndex) * 42;
        String ssid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);
        
        // 绘制背景
        if (i == selectedWiFiIndex) {
            tft.fillRoundRect(20, y - 12, SCREEN_WIDTH - 40, 36, 4, TFT_DARKGREY);
        }
        
        // 绘制信号强度
        int bars = map(rssi, -100, -30, 1, 5);
        for (int j = 0; j < bars; j++) {
            int barHeight = 5 + j * 5;
            tft.fillRect(30 + j * 10, y + 8 - barHeight, 8, barHeight, TFT_GREEN);
        }
        
        // 绘制SSID
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(95, y);
        tft.print(ssid.length() > 25 ? ssid.substring(0, 25) + "..." : ssid);
        
        // 绘制锁图标（如果加密）
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
            tft.setCursor(SCREEN_WIDTH - 50, y);
            tft.print("*");
        }
    }
    
    // 绘制上箭头按钮
    tft.fillRoundRect(150, 238, 60, 30, 4, wifiScrollOffset > 0 ? TFT_DARKGREY : TFT_DARKGREY);
    tft.setTextColor(wifiScrollOffset > 0 ? TFT_WHITE : TFT_LIGHTGREY);
    tft.setTextSize(2);
    tft.setCursor(175, 245);
    tft.print("^");
    
    // 绘制下箭头按钮
    tft.fillRoundRect(270, 238, 60, 30, 4, (wifiScrollOffset + maxVisible < wifiCount) ? TFT_DARKGREY : TFT_DARKGREY);
    tft.setTextColor((wifiScrollOffset + maxVisible < wifiCount) ? TFT_WHITE : TFT_LIGHTGREY);
    tft.setCursor(295, 245);
    tft.print("v");
}

void drawPasswordScreen() {
    tft.fillScreen(TFT_BLACK);
    
    // 绘制密码框
    tft.fillRoundRect(60, 50, 360, 35, 4, 0x2104);  // 深灰色背景
    tft.drawRoundRect(60, 50, 360, 35, 4, 0x4208);  // 边框
    tft.setTextColor(0x07E0);
    tft.setTextSize(2);
    tft.setCursor(70, 58);
    
    // 显示密码
    String displayPass = "";
    if (showPassword) {
        displayPass = wifiPasswordInput;
    } else {
        for (int i = 0; i < wifiPasswordInput.length(); i++) {
            displayPass += "*";
        }
    }
    tft.print(displayPass);
    
    // 绘制显示/隐藏密码按钮
    tft.fillRoundRect(385, 50, 35, 35, 4, 0x2104);
    tft.drawRoundRect(385, 50, 35, 35, 4, 0x4208);
    tft.setTextColor(0x8410);
    tft.setTextSize(2);
    tft.setCursor(393, 58);
    tft.print(showPassword ? "O" : "*");
    
    // 绘制虚拟键盘
    drawKeyboard();
}

void drawKeyboard() {
    int keySize = 36;
    int startX = 40;
    int startY = 95;
    int spacing = 4;
    
    tft.setTextSize(1);
    
    const char* currentKeyboard;
    int keyCount;
    
    switch (keyboardMode) {
        case 0:
            currentKeyboard = keyboardLower;
            keyCount = strlen(keyboardLower);
            break;
        case 1:
            currentKeyboard = keyboardUpper;
            keyCount = strlen(keyboardUpper);
            break;
        case 2:
            currentKeyboard = keyboardNumbers;
            keyCount = strlen(keyboardNumbers);
            break;
        case 3:
            currentKeyboard = keyboardSymbols;
            keyCount = strlen(keyboardSymbols);
            break;
        default:
            currentKeyboard = keyboardLower;
            keyCount = strlen(keyboardLower);
    }
    
    int keyIndex = 0;
    int maxCols = 10;
    int maxRows = (keyboardMode == 2) ? 1 : 3;
    
    for (int row = 0; row < maxRows; row++) {
        for (int col = 0; col < maxCols; col++) {
            if (keyIndex >= keyCount) break;
            
            int x = startX + col * (keySize + spacing);
            int y = startY + row * 36;
            
            // 绘制按键
            tft.fillRoundRect(x, y, keySize, 30, 3, 0x2104);
            tft.drawRoundRect(x, y, keySize, 30, 3, 0x3186);
            tft.setTextColor(TFT_WHITE);
            tft.setCursor(x + 12, y + 8);
            tft.print(currentKeyboard[keyIndex]);
            keyIndex++;
        }
    }
    
    // 绘制功能按钮
    int btnY = 224;
    
    // Shift按钮
    uint16_t shiftColor = (keyboardMode == 1) ? 0x4208 : 0x2104;
    tft.fillRoundRect(40, btnY, 65, 24, 4, shiftColor);
    tft.drawRoundRect(40, btnY, 65, 24, 4, 0x3186);
    tft.setTextColor(0x8410);
    tft.setTextSize(2);
    tft.setCursor(65, btnY + 5);
    tft.print("^");
    
    // 数字按钮
    uint16_t numColor = (keyboardMode == 2) ? 0x4208 : 0x2104;
    tft.fillRoundRect(115, btnY, 65, 24, 4, numColor);
    tft.drawRoundRect(115, btnY, 65, 24, 4, 0x3186);
    tft.setTextColor(0x8410);
    tft.setTextSize(1);
    tft.setCursor(130, btnY + 7);
    tft.print(keyboardMode == 2 ? "ABC" : "123");
    
    // 符号按钮
    uint16_t symColor = (keyboardMode == 3) ? 0x4208 : 0x2104;
    tft.fillRoundRect(190, btnY, 65, 24, 4, symColor);
    tft.drawRoundRect(190, btnY, 65, 24, 4, 0x3186);
    tft.setTextColor(0x8410);
    tft.setTextSize(1);
    tft.setCursor(205, btnY + 7);
    tft.print(keyboardMode == 3 ? "ABC" : "!@#");
    
    // 退格按钮
    tft.fillRoundRect(265, btnY, 85, 24, 4, 0x2104);
    tft.drawRoundRect(265, btnY, 85, 24, 4, 0x3186);
    tft.setTextColor(0x8410);
    tft.setTextSize(2);
    tft.setCursor(290, btnY + 5);
    tft.print("<");
    
    // 确认按钮
    tft.fillRoundRect(360, btnY, 80, 24, 4, 0x001F);
    tft.drawRoundRect(360, btnY, 80, 24, 4, 0x003F);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(385, btnY + 7);
    tft.print("OK");
}

bool handleKeyboardTouch(uint16_t touchX, uint16_t touchY) {
    // 检查显示/隐藏密码按钮
    if (touchX >= 385 && touchX <= 420 && touchY >= 50 && touchY <= 85) {
        showPassword = !showPassword;
        return true;
    }
    
    // 检查功能按钮
    int btnY = 224;
    
    // Shift按钮
    if (touchX >= 40 && touchX <= 105 && touchY >= btnY && touchY <= btnY + 24) {
        if (keyboardMode == 0) {
            keyboardMode = 1;
            lastAlphaMode = 1;
        } else if (keyboardMode == 1) {
            keyboardMode = 0;
            lastAlphaMode = 0;
        } else if (keyboardMode == 2 || keyboardMode == 3) {
            keyboardMode = lastAlphaMode;
            if (keyboardMode == 0) {
                keyboardMode = 1;
                lastAlphaMode = 1;
            } else {
                keyboardMode = 0;
                lastAlphaMode = 0;
            }
        }
        return true;
    }
    
    // 数字按钮
    if (touchX >= 115 && touchX <= 180 && touchY >= btnY && touchY <= btnY + 24) {
        if (keyboardMode == 2) {
            keyboardMode = lastAlphaMode;
        } else {
            if (keyboardMode == 0 || keyboardMode == 1) {
                lastAlphaMode = keyboardMode;
            }
            keyboardMode = 2;
        }
        return true;
    }
    
    // 符号按钮
    if (touchX >= 190 && touchX <= 255 && touchY >= btnY && touchY <= btnY + 24) {
        if (keyboardMode == 3) {
            keyboardMode = lastAlphaMode;
        } else {
            if (keyboardMode == 0 || keyboardMode == 1) {
                lastAlphaMode = keyboardMode;
            }
            keyboardMode = 3;
        }
        return true;
    }
    
    // 退格按钮
    if (touchX >= 265 && touchX <= 350 && touchY >= btnY && touchY <= btnY + 24) {
        if (wifiPasswordInput.length() > 0) {
            wifiPasswordInput.remove(wifiPasswordInput.length() - 1);
        }
        return true;
    }
    
    // 确认按钮
    if (touchX >= 360 && touchX <= 440 && touchY >= btnY && touchY <= btnY + 24) {
        wifiPassword = wifiPasswordInput;
        connectToWiFi();
        return true;
    }
    
    // 检查按键
    int keySize = 36;
    int startX = 40;
    int startY = 95;
    int spacing = 4;
    
    const char* currentKeyboard;
    int keyCount;
    
    switch (keyboardMode) {
        case 0:
            currentKeyboard = keyboardLower;
            keyCount = strlen(keyboardLower);
            break;
        case 1:
            currentKeyboard = keyboardUpper;
            keyCount = strlen(keyboardUpper);
            break;
        case 2:
            currentKeyboard = keyboardNumbers;
            keyCount = strlen(keyboardNumbers);
            break;
        case 3:
            currentKeyboard = keyboardSymbols;
            keyCount = strlen(keyboardSymbols);
            break;
        default:
            currentKeyboard = keyboardLower;
            keyCount = strlen(keyboardLower);
    }
    
    int keyIndex = 0;
    int maxCols = 10;
    int maxRows = (keyboardMode == 2) ? 1 : 3;
    
    for (int row = 0; row < maxRows; row++) {
        for (int col = 0; col < maxCols; col++) {
            if (keyIndex >= keyCount) break;
            
            int x = startX + col * (keySize + spacing);
            int y = startY + row * 36;
            
            if (touchX >= x && touchX <= x + keySize && 
                touchY >= y && touchY <= y + 30) {
                wifiPasswordInput += currentKeyboard[keyIndex];
                return true;
            }
            keyIndex++;
        }
    }
    return false;
}

void scanWiFi() {
    drawStatusScreen("正在扫描WiFi...", "请稍候");
    wifiCount = WiFi.scanNetworks();
    wifiCount = min(wifiCount, 10); // 最多显示10个
    selectedWiFiIndex = 0;
    wifiScrollOffset = 0;
    configState = STATE_SELECT;
    drawWiFiList();
}

void connectToWiFi() {
    configState = STATE_CONNECTING;
    drawStatusScreen("正在连接...", wifiSSID.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    unsigned long start = millis();
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
        dotCount = (dotCount + 1) % 4;
        String dots = "";
        for (int i = 0; i < dotCount; i++) dots += ".";
        
        tft.fillScreen(TFT_BLACK);
        drawTextCenter("ECAN赛车无线屏", 50, TFT_RED);
        tft.setTextSize(1);
        drawTextCenter("正在连接WiFi", 100, TFT_WHITE);
        drawTextCenter(String(wifiSSID + dots).c_str(), 135, TFT_SILVER);
        
        delay(300);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        saveWiFiConfig(wifiSSID.c_str(), wifiPassword.c_str());
        currentMode = MODE_WORKING;
        local_IP = WiFi.localIP();
        showIPAddress();  // 显示IP直到有推流
    } else {
        drawStatusScreen("连接失败", "请重试");
        delay(2000);
        configState = STATE_SELECT;
        drawWiFiList();
    }
}

void handleConfigTouch() {
    uint16_t touchX = 0, touchY = 0;
    
    // 这里暂时禁用触摸检测，后续需要根据实际硬件调整
    // if (tft.getTouch(&touchX, &touchY)) {
    // 简化版本，暂时不实现触摸交互
}

bool connectToSavedWiFi() {
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();
    
    if (ssid.length() == 0) return false;
    
    drawStatusScreen("正在连接WiFi...", String("WiFi: " + ssid).c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long start = millis();
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
        // 显示连接动画
        dotCount = (dotCount + 1) % 4;
        String dots = "";
        for (int i = 0; i < dotCount; i++) dots += ".";
        
        tft.fillScreen(TFT_BLACK);
        drawTextCenter("ECAN赛车无线屏", 50, TFT_RED);
        tft.setTextSize(1);
        drawTextCenter("正在连接WiFi", 100, TFT_WHITE);
        drawTextCenter(String("WiFi: " + ssid + dots).c_str(), 135, TFT_SILVER);
        
        delay(300);
    }
    return (WiFi.status() == WL_CONNECTED);
}

void showIPAddress() {
    tft.fillScreen(TFT_BLACK);
    
    // 绘制提示文字
    tft.setTextColor(0x8410);
    tft.setTextSize(1);
    String hint1 = "请在PC端软件填写此IP";
    int textWidth = tft.textWidth(hint1);
    tft.setCursor((SCREEN_WIDTH - textWidth) / 2, 55);
    tft.print(hint1);
    
    // 绘制IP地址框背景
    tft.fillRoundRect(90, 65, 300, 40, 4, 0x2104);
    tft.drawRoundRect(90, 65, 300, 40, 4, 0x3186);
    
    // 绘制IP地址
    tft.setTextColor(0x001F);
    tft.setTextSize(2);
    String ipText = "IP: " + local_IP.toString();
    textWidth = tft.textWidth(ipText);
    tft.setCursor((SCREEN_WIDTH - textWidth) / 2, 77);
    tft.print(ipText);
    
    // 绘制官网
    tft.setTextColor(0x8410);
    tft.setTextSize(1);
    String websiteText = "官网：WWW.ECANNON.CC";
    textWidth = tft.textWidth(websiteText);
    tft.setCursor((SCREEN_WIDTH - textWidth) / 2, 120);
    tft.print(websiteText);
    
    // 绘制成功图标
    tft.fillCircle(240, 170, 55, 0x0620);
    tft.fillCircle(240, 170, 48, 0x0420);
    tft.fillCircle(240, 170, 40, 0x0220);
    tft.fillCircle(240, 170, 30, TFT_BLACK);
    
    // 绘制对勾
    tft.drawLine(215, 165, 235, 185, 0x07E0);
    tft.drawLine(235, 185, 265, 155, 0x07E0);
}

void saveWiFiConfig(const char* ssid, const char* password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

void clearWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
}

void startAPConfig() {
    currentMode = MODE_CONFIG_AP;
    configStartTime = millis();
    
    String apSSID = AP_SSID_BASE + String(random(1000, 9999));
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    
    drawStatusScreen("无触屏配网模式", String("热点名: " + apSSID).c_str());
}

void startTouchConfig() {
    currentMode = MODE_CONFIG_TOUCH;
    configStartTime = millis();
    configState = STATE_SCAN;
    scanWiFi();
}

void resetConfig() {
    clearWiFiConfig();
    ESP.restart();
}

int JPEGDraw(JPEGDRAW* pDraw) {
    tft.startWrite();
    tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, (uint16_t*)pDraw->pPixels);
    tft.endWrite();
    return 1;
}

void handleUDP() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        remoteIP = udp.remoteIP();
        remotePort = udp.remotePort();
        
        static uint8_t* buffer = nullptr;
        static int bufferSize = 0;
        
        // 预分配更大的缓冲区，避免重复malloc
        if (bufferSize < packetSize + 1024) {
            if (buffer) free(buffer);
            bufferSize = packetSize + 4096;
            buffer = (uint8_t*)ps_malloc(bufferSize);
        }
        
        int len = udp.read(buffer, packetSize);
        if (len > 0) {
            // 立即解码显示，不做额外处理
            jpeg.openRAM(buffer, len, JPEGDraw);
            jpeg.decode(0, 0, 0);
            jpeg.close();
            lastHeartbeat = millis();
            lastFrameTime = millis();
            firstFrameReceived = true;  // 标记已收到第一帧
        }
    }
}

void loop() {
    resetButton.tick();
    
    switch (currentMode) {
        case MODE_WORKING: {
            if (WiFi.status() != WL_CONNECTED) {
                reconnectAttempts++;
                if (reconnectAttempts > WIFI_RECONNECT_ATTEMPTS) {
                    if (hasTouch) startTouchConfig();
                    else startAPConfig();
                } else {
                    WiFi.reconnect();
                }
            } else {
                // 优先处理UDP接收（图像数据）
                handleUDP();
                
                // 检测触摸事件
                int currentTouchEvent = detectTouchEvent();
                unsigned long currentTime = millis();
                
                // 触摸按下检测（消抖处理）
                if (currentTouchEvent != TOUCH_EVENT_NONE && !lastTouchState) {
                    if (currentTime - lastTouchTime > 200) {  // 200ms消抖
                        sendTouchEvent(currentTouchEvent);
                        lastTouchEvent = currentTouchEvent;
                        lastTouchState = true;
                        lastTouchTime = currentTime;
                    }
                }
                
                // 触摸释放检测
                if (currentTouchEvent == TOUCH_EVENT_NONE && lastTouchState) {
                    lastTouchState = false;
                    lastTouchEvent = TOUCH_EVENT_NONE;
                }
                
                // 非阻塞发送心跳（每1秒一次）
                if (remotePort > 0) {
                    unsigned long now = millis();
                    if (now - lastHeartbeatSend >= 1000) {
                        const char* heartbeatMsg = "ESP32_HEARTBEAT";
                        udp.beginPacket(remoteIP, remotePort);
                        udp.write((const uint8_t*)heartbeatMsg, strlen(heartbeatMsg));
                        udp.endPacket();
                        lastHeartbeatSend = now;
                    }
                }
            }
            break;
        }
        case MODE_CONFIG_AP: {
            if (millis() - configStartTime > CONFIG_TIMEOUT) {
                ESP.restart();
            }
            break;
        }
        case MODE_CONFIG_TOUCH: {
            if (millis() - configStartTime > CONFIG_TIMEOUT) {
                ESP.restart();
            }
            handleConfigTouch();
            break;
        }
        case MODE_WAIT:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    tft.begin();
    tft.setRotation(1);
    
    detectTouch();
    
    resetButton.attachClick([]() {});
    resetButton.attachLongPressStart(resetConfig);
    
    if (connectToSavedWiFi()) {
        currentMode = MODE_WORKING;
        local_IP = WiFi.localIP();
        showIPAddress();  // 显示IP直到有推流
    } else if (hasTouch) {
        startTouchConfig();
    } else {
        startAPConfig();
    }
    
    udp.begin(UDP_PORT);
}
