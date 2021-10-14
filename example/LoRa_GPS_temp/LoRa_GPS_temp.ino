#include <Arduino.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "gps.h"
#include "SoftwareSerial2.h"
TFT_eSPI tft = TFT_eSPI();           // Invoke custom library
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite

SoftwareSerial1 softSerial2(40, 41);

#include "DHT.h"
#define DHTPIN 0      // what pin we're connected to
#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//TFT_eSPI tft;
#include "fonts.h"
#define X_OFFSET 2
#define Y_OFFSET 0
#define X_SIZE 80
#define Y_SIZE 20
#define R_SIZE 4
#define BOX_SPACING 2

#define TFT_GRAY 0b1010010100010000
#define TFT_GRAY10 0b0100001100001000
#define TFT_GRAY20 0b0010000110000100

#define HIST_X_OFFSET 2
#define HIST_Y_OFFSET 75
#define HIST_X_SIZE 315
#define HIST_X_TXTSIZE X_SIZE - 3
#define HIST_Y_SIZE 160
#define HIST_X_BAR_OFFSET 50
#define HIST_X_BAR_SPACE 2

static int P_temp, P_humi, P_is_exist = 1, P_is_join = 1;
static char recv_buf[512];
static bool is_exist = false;
static bool is_join = false;
static int led = 0;
static bool Lora_is_busy = false;
static int temp = 0;
static int humi = 0;
static int switch_UI = 0;

char str[100] = {0};
char str_devEui[50] = {0};
char str_appEvi[50] = {0};
//static int  Lora_Ack_timeout;

enum e_module_Response_Result{
  MODULE_IDLE,
  MODULE_RECEIVING,
  MODULE_TIMEOUT,
  MODULE_ACK_SUCCESS,
};

enum e_module_AT_Cmd{
  AT_OK,
  AT_ID,
  AT_MODE,
  AT_DR,
  AT_CH,
  AT_KEY,
  AT_CLASS,
  AT_PORT,
  AT_JOIN,
  AT_CMSGHEX,
  AT_TIMOUT,
};

int module_AT_Cmd = 0;
typedef struct s_E5_Module_Cmd{
  char p_ack[15];
  int timeout_ms;
  char p_cmd[70];
} E5_Module_Cmd_t;
E5_Module_Cmd_t E5_Module_Cmd[] ={
  {"+AT: OK", 1000, "AT\r\n"},
  {"+ID: AppEui", 1000, "AT+ID\r\n"},
  {"+MODE", 1000, "AT+MODE=LWOTAA\r\n"},
  {"+DR", 1000, "AT+DR=EU868\r\n"},
  {"CH", 1000, "AT+CH=NUM,0-2\r\n"},
  {"+KEY: APPKEY", 1000, "AT+KEY=APPKEY,\"2B7E151628AED2A6ABF7158809CF4F3C\"\r\n"},
  {"+CLASS", 1000, "AT+CLASS=A\r\n"},
  {"+PORT", 1000, "AT+PORT=8\r\n"},
  {"Done", 10000, "AT+JOIN\r\n"},
  {"Done", 30000, ""},
};

static int at_send_check_response(char *p_ack, int timeout_ms, char *p_cmd, ...){
  int ch;
  int num = 0;
  int index = 0;
  int startMillis = 0;
  va_list args;
  memset(recv_buf, 0, sizeof(recv_buf));
  va_start(args, p_cmd);
  softSerial2.printf(p_cmd, args);
  Serial.printf(p_cmd, args);
  va_end(args);

  startMillis = millis();

  if (p_ack == NULL){
    return 0;
  }
  do{
    while (softSerial2.available() > 0)
    {
      ch = softSerial2.read();
      recv_buf[index++] = ch;
      Serial.print((char)ch);
      delay(2);
    }

    if (strstr(recv_buf, p_ack) != NULL){
      return 1;
    }

  } while (millis() - startMillis < timeout_ms);
  return 0;
}

static int at_send_check_response_flag(int timeout_ms, char *p_cmd, ...){
  if (Lora_is_busy == true){
    return 0;
  }
  Lora_is_busy = true;
//  Lora_Ack_timeout = timeout_ms;
  va_list args;
  memset(recv_buf, 0, sizeof(recv_buf));
  va_start(args, p_cmd);
  softSerial2.printf(p_cmd, args);
  Serial.printf(p_cmd, args);
  va_end(args);
  return 1;
}

static int check_message_response(){
  static bool init_flag = false;
  static int startMillis = 0;
  static int index = 0;
  e_module_Response_Result result = MODULE_ACK_SUCCESS;
  int ch;
  int num = 0;
  if (Lora_is_busy == true){
    if (init_flag == false){
      startMillis = millis();
      init_flag = true;
      index = 0;
      memset(recv_buf, 0, sizeof(recv_buf));
      Serial.println("Cmd Start......");
    }
    Lora_is_busy = false;
    init_flag = false;
    while (softSerial2.available() > 0){
      ch = softSerial2.read();
      recv_buf[index++] = ch;
      Serial.print((char)ch);
      delay(2);
    }

    if (strstr(recv_buf, E5_Module_Cmd[module_AT_Cmd].p_ack) != NULL){
      //          is_join = true;
      return result;
    }

    if (millis() - startMillis >= E5_Module_Cmd[module_AT_Cmd].timeout_ms){
      Serial.println("Cmd Timeout......");
      return MODULE_TIMEOUT;
    }
    Lora_is_busy = true;
    init_flag = true;
    return MODULE_RECEIVING;
  }
  return MODULE_IDLE;
}

static int init_ui = 0;
void display_EVI(){
  //  static int init_ui = 0;
  if (init_ui == 0){
    init_ui = 1;
    tft.setFreeFont(FMO12);
    tft.fillRect(0, 0, 320, 30, TFT_BLUE);      //Drawing rectangle with blue fill
    tft.setTextColor(TFT_WHITE);                //Setting text color
    tft.drawString("GPS & temperature", 35, 5); //Drawing text string


    tft.setFreeFont(FM9);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Device", 5, 40, GFXFF);
    tft.drawRoundRect(HIST_X_OFFSET, HIST_Y_OFFSET - 20, HIST_X_SIZE, HIST_Y_SIZE + 20, R_SIZE, TFT_WHITE);

    tft.setFreeFont(FSSO9);
    tft.setTextColor(TFT_BLUE);
    tft.drawString("LoRaWAN", HIST_X_OFFSET + 10, HIST_Y_OFFSET , GFXFF);
    tft.setFreeFont(FM9);
    tft.setTextColor(TFT_WHITE);
    tft.fillRect(HIST_X_OFFSET + 10, HIST_Y_OFFSET + 25, 300, 15, TFT_BLACK);
    strcpy(str, "DevEui:");
    strcat(str, str_devEui);
    tft.drawString(str, HIST_X_OFFSET + 10, HIST_Y_OFFSET + 25, GFXFF);
    memset(str, 0, sizeof(str));
    tft.fillRect(HIST_X_OFFSET + 10, HIST_Y_OFFSET + 45, 300, 15, TFT_BLACK);
    strcpy(str, "AppEui:");
    strcat(str, str_appEvi);
    tft.drawString(str, HIST_X_OFFSET + 10, HIST_Y_OFFSET + 45, GFXFF);
    tft.drawString("AppKey:", HIST_X_OFFSET + 10, HIST_Y_OFFSET + 65, GFXFF);
    tft.drawString("\"", HIST_X_OFFSET + 10, HIST_Y_OFFSET + 65, GFXFF);
    tft.drawString("2B7E151628AED2A6AB", HIST_X_OFFSET + 88, HIST_Y_OFFSET + 65, GFXFF);
    tft.drawString("F7158809CF4F3C", HIST_X_OFFSET + 88, HIST_Y_OFFSET + 85, GFXFF);
  }
}



void refreshGpsInfo(){
  int xOffset, yOffset;
//  static int init_ui = 0;
  if (init_ui == 0){
    tft.fillRect(HIST_X_OFFSET, HIST_Y_OFFSET - 18, HIST_X_TXTSIZE, 18, TFT_BLACK);
    tft.fillRect(HIST_X_OFFSET, HIST_Y_OFFSET, HIST_X_SIZE, HIST_Y_SIZE, TFT_BLACK);

    tft.setFreeFont(FMO12);
    tft.fillRect(0, 0, 320, 30, TFT_BLUE);      //Drawing rectangle with blue fill
    tft.setTextColor(TFT_WHITE);                //Setting text color
    tft.drawString("GPS & temperature", 35, 5); //Drawing text string
    tft.setFreeFont(&FreeMono9pt7b);
    tft.drawString("Device:", 5, 45);
    tft.drawString("STATE:", 146, 45);

    tft.setFreeFont(&FreeMono9pt7b);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Temp:", 5, 88, 1); // Print the test text in the custom font
    tft.drawRoundRect(65, 75, 80, 40, 5, TFT_WHITE);
    tft.drawString("Humi:", 165, 88, 1); // Print the test text in the custom font
    tft.drawRoundRect(225, 75, 80, 40, 5, TFT_WHITE);

    tft.setFreeFont(&FreeSansBoldOblique9pt7b);
    tft.drawString("C", 120, 85, 1);
    tft.drawString("%", 280, 85, 1);

    tft.setFreeFont(FM9);
    tft.setTextColor(TFT_WHITE);

    xOffset = HIST_X_OFFSET + 10;
    yOffset = HIST_Y_OFFSET + 50;
    tft.drawString("Date: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_date, xOffset + 50, yOffset, GFXFF);
    yOffset += 18;
    tft.drawString("Time: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_time, xOffset + 50, yOffset, GFXFF);
    yOffset += 18;
    tft.drawString("LAT: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_lat, xOffset + 50, yOffset, GFXFF);
    yOffset += 18;
    tft.drawString("LONG: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_lng, xOffset + 50, yOffset, GFXFF);
    yOffset += 18;
    tft.drawString("ALT: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_meters, xOffset + 50, yOffset, GFXFF);
    yOffset += 18;
    tft.drawString("Satellites: ", xOffset, yOffset, GFXFF);
    tft.drawString(N_satellites, xOffset + 120, yOffset, GFXFF);

    tft.drawRoundRect(HIST_X_OFFSET, HIST_Y_OFFSET + 45, HIST_X_SIZE, HIST_Y_SIZE - 45, R_SIZE, TFT_WHITE);
    //    ui.previous_display = ui.selected_display;
  }

  if (is_exist != P_is_exist || init_ui == 0){
    if (is_exist){
      tft.fillRect(80, 45, 65, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.drawString("find", 80, 45, GFXFF);
    }
    else{
      tft.fillRect(80, 45, 65, 15, TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.drawString("unfind", 80, 45, GFXFF);
    }
    P_is_exist = is_exist;
  }

  if (is_join != P_is_join || init_ui == 0){
    if (is_join){
      tft.fillRect(210, 45, 110, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.drawString("connected", 210, 45, GFXFF);
    }
    else{
      tft.fillRect(210, 45, 110, 15, TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.drawString("disconnect", 210, 45, GFXFF);
    }
    P_is_join = is_join;
  }


  tft.setFreeFont(FM9);
  tft.setTextColor(TFT_WHITE);
  xOffset = HIST_X_OFFSET + 10 + 50;
  yOffset = HIST_Y_OFFSET + 50;


  if (temp != P_temp || init_ui == 0){
    tft.fillRect(80, 88, 25, 25, TFT_BLACK);
    tft.drawNumber(temp, 80, 88, GFXFF);
    P_temp = temp;
  }

  if (humi != P_humi || init_ui == 0){
    tft.fillRect(240, 88, 25, 25, TFT_BLACK);
    tft.drawNumber(humi, 240, 88, GFXFF);
    P_humi = humi;
  }

  if (N_date != P_date){
    tft.fillRect(xOffset, yOffset, 150, 18, TFT_BLACK);
    tft.drawString(N_date, xOffset + 15, yOffset, GFXFF);
    P_date = N_date;
  }
  yOffset += 18;
  if (N_time != P_time){
    tft.fillRect(xOffset, yOffset, 150, 18, TFT_BLACK);
    tft.drawString(N_time, xOffset + 15, yOffset, GFXFF);
    P_time = N_time;
  }
  yOffset += 18;
  if (N_lat != P_lat){
    tft.fillRect(xOffset, yOffset, 150, 18, TFT_BLACK);
    tft.drawString(N_lat, xOffset + 15, yOffset, GFXFF);
    P_lat = N_lat;
  }
  yOffset += 18;
  if (N_lng != P_lng){
    tft.fillRect(xOffset, yOffset, 150, 18, TFT_BLACK);
    tft.drawString(N_lng, xOffset + 15, yOffset, GFXFF);
    P_lng = N_lng;
  }
  yOffset += 18;
  if (N_meters != P_meters){
    tft.fillRect(xOffset, yOffset, 150, 18, TFT_BLACK);
    tft.drawString(N_meters, xOffset + 15, yOffset, GFXFF);
    P_meters = N_meters;
  }
  xOffset += 70;
  yOffset += 18;
  if (N_satellites != P_satellites){
    tft.fillRect(xOffset, yOffset, 100, 18, TFT_BLACK);
    tft.drawString(N_satellites, xOffset + 15, yOffset, GFXFF);
    P_satellites = N_satellites;
  }
    init_ui = 1;
}


void setup(void){
  tft.begin();
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  Serial.begin(115200);

  delay(2000);
  GpsSerialInit();

  softSerial2.begin(9600);
  softSerial2.listen();
  while (!softSerial2);
  Serial.print("E5 LORAWAN TEST\r\n");
  dht.begin();
  
}


void loop(void){
  static long cTime = 5 * 60 * 1000; //0;
  static long tTime = 5 * 60 * 1000; //0;
  long sTime = millis();
  

  if (tTime > 10 * 1000){
    tTime = 0;
    GpsstopListening();
    temp = dht.readTemperature();
    humi = dht.readHumidity();
    GpsSerialInit();
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" *C");
  }

  if(switch_UI == 0){
    refreshGpsInfo();
    }

    if(switch_UI == -1){
    display_EVI();
    }  

  if (digitalRead(WIO_5S_LEFT) == LOW) {
    tft.fillScreen(TFT_BLACK);
    init_ui = 0;  
    switch_UI = -1;
    
  }

  if (digitalRead(WIO_5S_RIGHT) == LOW) {
    tft.fillScreen(TFT_BLACK);
    init_ui = 0;  
    switch_UI = 0;
  }

  GetGpsInfoPolling();
  UpdateGpsInfo();

  switch (check_message_response()){
    case MODULE_IDLE:
      if (module_AT_Cmd <= AT_PORT){
        at_send_check_response_flag(E5_Module_Cmd[module_AT_Cmd].timeout_ms, E5_Module_Cmd[module_AT_Cmd].p_cmd);
        Serial.print("module_AT_Cmd = ");
        Serial.println(module_AT_Cmd);
      }
      else if (is_join == true){
        if (cTime > 30 * 1000)
        {
          cTime = 0;
          char cmd[128];
          module_AT_Cmd = AT_CMSGHEX;
          sprintf(cmd, "AT+CMSGHEX=\"%04X%04X%08X%08X%08X%02X\"\r\n", (int)temp, (int)humi, (int)(Lat*1), (int)(Lng*1),(int)(Meters*1),(int)Satellites);
          
          at_send_check_response_flag(E5_Module_Cmd[module_AT_Cmd].timeout_ms, cmd);
          Serial.println("Send Data");
        }
      }
      else if (is_join == false){
        if (cTime > 5 * 1000){
          Serial.println("Send Jion");
          cTime = 0;
          module_AT_Cmd = AT_JOIN;
          at_send_check_response_flag(E5_Module_Cmd[module_AT_Cmd].timeout_ms, E5_Module_Cmd[module_AT_Cmd].p_cmd);
        }
      }

      break;
    case MODULE_RECEIVING:
      break;
    case MODULE_TIMEOUT:
      is_exist = false;
      is_join  = false;
      module_AT_Cmd = AT_OK;
      Serial.println("MODULE_TIMEOUT");
      break;
    case MODULE_ACK_SUCCESS:
      is_exist = true;

      switch (module_AT_Cmd){
        case AT_JOIN:
          if (strstr(recv_buf, "Network joined") != NULL){
            is_join = true;
          }
          else{
            is_join = false;
          }
          break;

        case AT_ID:
          int j = 0;
          char *p_start = NULL;
          p_start = strstr(recv_buf, "DevEui");
          sscanf(p_start, "DevEui, %23s,", &str);//&E5_Module_Data.DevEui);
          j = 0;
          for (int i = 0; i < 16; i++, j++){
            if ((i != 0) && (i % 2 == 0)){
              j += 1;
            }
            str_devEui[i] = str[j];
          }
          Serial.println(str_devEui);
          p_start = strstr(recv_buf, "AppEui");
          sscanf(p_start, "AppEui, %23s,", &str);//&E5_Module_Data.AppEui);
          j = 0;
          for (int i = 0; i < 16; i++, j++){
            if ((i != 0) && (i % 2 == 0)){
              j += 1;
            }
            str_appEvi[i] = str[j];
          }
          Serial.println(str_appEvi);
          init_ui = 0;
          break;

      }

      if (module_AT_Cmd <= AT_PORT){
        module_AT_Cmd++;
      }
      break;
  }
  //  delay(10);
  long duration = millis() - sTime;
  //  if ( duration < 0 ) duration = 10;
  cTime += duration;
  tTime += duration;
}
