#include "esp_camera.h"
#include <WiFi.h>

#include <vector>
#include "AsyncUDP.h"
#include "esp_netif.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include <lwip/netdb.h>


//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE  // Has PSRAM
#define CAMERA_MODEL_AI_THINKER
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "CMCC-Frwt";
const char *password = "u69rwpm3";

#define MAX_SRV_CLIENTS 20
const char* ap_ssid = "my_esp32";        //设置自身WiFi热点账号
const char* ap_password = "zw19980919";  //设置自身WiFi热点密码
uint8_t str[]={"Happy New Year !"};      //给客户端传输的数据
IPAddress staticIP(192, 168, 4, 1);      //自身ip
IPAddress gateway(192, 168, 4, 1);       //网关ip
IPAddress subnet(255, 255, 255, 0);      //子网掩码

int32_t blink_delay = 1000;

AsyncUDP udp;                            //异步udp既可以发送也可以接收



void startCameraServer();
void setupLedFlash(int pin);
void camera_init(void);
#define LED_GPIO_NUM 4
#define maxcache 1024


void setup() 
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  //WiFi.mode(WIFI_AP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  // if(WiFi.softAPConfig(staticIP, gateway, subnet)==false)// 设置静态IP地址
  // {
  //   Serial.println("Configuration failed.");
  // }

  // IPAddress ip = WiFi.softAPIP();                        // 获取AP的IP地址
  // WiFi.softAP(ap_ssid, ap_password,1,0,4);                     //设置热点为显现，最大连接数量为4，生成WiFi热点
  // Serial.print("ap wifi ip:");
  // Serial.println(WiFi.softAPIP());
  pinMode(LED_GPIO_NUM,OUTPUT);
  camera_init();
  xTaskCreate(UdpSendCameraData, "UdpSendCameraData", 100*1024, (void *)&blink_delay, 2, NULL);


  while (WiFi.status() != WL_CONNECTED) {  
    delay(500);  
    Serial.print(".");  
  }  
  
  // WiFi连接成功后的操作  
  Serial.println("");  
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");  
  Serial.println(WiFi.localIP());  

  //startCameraServer();
  // Serial.print("Camera Ready! Use 'http://");
  // Serial.print(WiFi.localIP());
  // Serial.println("' to connect");
 
}

void loop()
{
  
  // digitalWrite(LED_GPIO_NUM,LOW);
  // delay(1000);
  // digitalWrite(LED_GPIO_NUM,HIGH);
  //  delay(1000);
  // camera_fb_t * fb = esp_camera_fb_get();
  // for(int i = 0; i < fb->len; i++)
  //   {
  //       Serial.write(fb->buf[i]); // 错误的方法
  //   }

  //  Serial.print("fb-buf-len:");
  //  Serial.println(fb->len);
  //  esp_camera_fb_return(fb); 
  delay(10000);

}



void camera_init(void)
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

}

void UdpSendCameraData(void* data)
{
  WiFiUDP Udp;
  IPAddress remote_IP(192, 168, 1, 2);            //windows的ip地址
  unsigned int remoteUdpPort = 6000;              //windows的udp端口号
  //Udp.beginPacket(remote_IP, remoteUdpPort);      //配置远端ip地址和端口
  int32_t cnt = 0;
  unsigned char *fb_data = NULL; 
  int32_t len = 0;
  //udp.connect(IPAddress(192,168,1,2), 6000);


  struct sockaddr_in socket_addr;
  socket_addr.sin_addr.s_addr = inet_addr("192.168.1.2");
  socket_addr.sin_family = AF_INET;
  socket_addr.sin_port = htons(6000);
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0)
  {
      Serial.println("Unable to create socket");
      close(sock);
  return;
  }

  //int err = bind(sock, (struct sockaddr *)&socket_addr, sizeof(socket_addr));
  // if(err < 0)
  // {
  //    Serial.println("Socket unable to bind:");
  //    close(sock);
  // }

  
  while(1)
  {
      camera_fb_t * fb = esp_camera_fb_get();


      fb_data = (unsigned char *)malloc(fb->len + 8);          //前面四个字节是长度，后面四个字节是结束符
      // len = fb->len + 8;
      // int timess = len / maxcache;
      // int extra = len % maxcache;
      // int i = 0;

      *(unsigned int*)fb_data = fb->len + 8;  // 设置总长度（注意：这里假设了 sizeof(unsigned int) == 4）  
      memcpy(fb_data + 4, fb->buf, fb->len);  // 复制图像数据到fb_data的偏移位置  
      memset(fb_data + 4 + fb->len, 0xFF, 4); // 设置结束符  

      Serial.print("fb-buf-len:");
      Serial.println(fb->len);
     
      //Serial.println(sizeof(temparray));
    //   for(int i = 0; i < fb->len; i++)
    // {
    //     Serial.write(fb->buf[i]); // 错误的方法
    // }

      //  Udp.beginPacket(remote_IP, remoteUdpPort); 
      //  Udp.write((const uint8_t*)fb_data, fb->len + 8); //复制数据到发送缓存
      // // //Udp.write((const uint8_t*)fb->buf, fb->len); //复制数据到发送缓存
      //  Udp.endPacket();//发送数据

      // for(i = 0; i < timess; i++)
      // {
      //     udp.write((const uint8_t*)(fb_data + i * maxcache), maxcache);
      // }
      // if(0 != extra)
      // {
      //     udp.write((const uint8_t*)(fb_data + i * maxcache), extra);
      // }

      int err = sendto(sock, fb_data, fb->len + 8, 0, (struct sockaddr *)&socket_addr, sizeof(socket_addr));
      if(err < 0) 
      {
        Serial.println("Error occurred during sending:");
        Serial.println(errno);
        //close(sock);
      }


      free(fb_data);
      esp_camera_fb_return(fb); 

      Serial.println("IP address: ");  
      Serial.println(WiFi.localIP());  
    
      
      //Serial.println("my name is UdpSendCameraData");
      //delay(10);
  }
}



