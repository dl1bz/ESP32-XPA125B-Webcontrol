#define VERSION "XPA125B Web-Control V1.0 by DL1BZ, 2023"

// Import required libraries:
// Arduino_JSON library by Arduino version 0.2.0 (Arduino Library Manager)
// ESPAsyncWebServer (.zip folder, Github)
// AsyncTCP (.zip folder, Github)

// ideas I use and adopt from other projects:
// https://randomnerdtutorials.com/esp32-websocket-server-arduino/
// https://randomnerdtutorials.com/esp32-websocket-server-sensor/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

// For this project here we need an ESP32 with Wifi integrated
// I use a LILYGO ESP32 TTGO T-DISPLAY v1.1, if you use other ESP32, look in it's datasheet or description, Pin numbers
// may be different

#define TDISPLAY            // if we have a T-DISPLAY and want an output to TFT - otherweise please comment out this line  

#ifdef TDISPLAY
#include <TFT_eSPI.h>       //using this LIB https://github.com/Xinyuan-LilyGO/TTGO-T-Display/tree/master/TFT_eSPI  
// IMPORTANT!  
//      In the "User_Setup_Select.h" file, enable "#include <User_Setups/Setup25_TTGO_T_Display.h>"
//-----------------------------------------------------------------------------------------
//for TFT
TFT_eSPI tft = TFT_eSPI();

#define screen_width  240       //placement of text etc must fit withing these boundaries.
#define screen_heigth 135

// all my known colors for ST7789 TFT (but not all used in program)
#define B_DD6USB 0x0004    //   0,   0,   4
#define BLACK 0x0000       //   0,   0,   0
#define NAVY 0x000F        //   0,   0, 123
#define DARKGREEN 0x03E0   //   0, 125,   0
#define DARKCYAN 0x03EF    //   0, 125, 123
#define MAROON 0x7800      // 123,   0,   0
#define PURPLE 0x780F      // 123,   0, 123
#define OLIVE 0x7BE0       // 123, 125,   0
#define LIGHTGREY 0xC618   // 198, 195, 198
#define DARKGREY 0x7BEF    // 123, 125, 123
#define BLUE 0x001F        //   0,   0, 255
#define GREEN 0x07E0       //   0, 255,   0
#define CYAN 0x07FF        //   0, 255, 255
#define RED 0xF800         // 255,   0,   0
#define MAGENTA 0xF81F     // 255,   0, 255
#define YELLOW 0xFFE0      // 255, 255,   0
#define WHITE 0xFFFF       // 255, 255, 255
#define ORANGE 0xFD20      // 255, 165,   0
#define GREENYELLOW 0xAFE5 // 173, 255,  41
#define PINK 0xFC18        // 255, 130, 198
//*************************************************************

//=================================================
// Mapping of port-pins to functions on ESP32
//=================================================

// the Pins for SPI
#define TFT_CS    5
#define TFT_DC   16
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_RST  23
#define TFT_BL    4

//-----------------------------------------------------------------------------------------
// initialise the TFT display
//-----------------------------------------------------------------------------------------

void init_TFT(void)
{
    //tft.init(screen_heigth, screen_width) ;  //not used

    tft.init();
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);              // switch backlight on

    tft.fillScreen(BLACK);
    tft.setRotation(0);
    tft.fillRoundRect(0, 0, tft.width(), 30, 5, MAROON);   // background for screen title
    tft.drawRoundRect(0, 0, tft.width(), 30, 5, WHITE);    //with white border.

    tft.setFreeFont(NULL);               // Set default font
    tft.setTextSize(2);                  //for default Font only.Font is later changed.
    tft.setTextColor(YELLOW);
    tft.setCursor(25, 7);                //top line
    tft.print("XPA125B");
    tft.fillRect(0, 90, 135, 150, BLUE);
    tft.setTextColor(WHITE);            //white from now on
 
}

void statusWIFI_TFT() {
  tft.setTextSize(1);
  tft.setCursor(25, 41);
  tft.print(WiFi.localIP());
  tft.setCursor(25,51);
  tft.print(WiFi.SSID());
  tft.setCursor(25,61);
  tft.print("RSSI: "); tft.print(WiFi.RSSI()); tft.print("dbm");
}

void showDATA_TFT(String fwdpwr, String refpwr, String swr) {
  // tft.setFreeFont(&FreeSansBold9pt7b);
  // tft.setTextSize(2);
  // tft.setFreeFont(&Orbitron_Light_32);
  tft.setFreeFont(&Orbitron_Light_24); // select font
  tft.setTextSize(1);                  // select text size
  tft.fillRect(0, 90, 135, 150, BLUE); // clear part of screen before update the power values
  tft.setCursor(5, 130); tft.print("F");
  tft.setCursor(35, 130); tft.print(fwdpwr);
  tft.setCursor(80, 130); tft.print(" W");
  tft.setCursor(5, 160); tft.print("R");
  tft.setCursor(35, 160); tft.print(refpwr);
  tft.setCursor(80, 160); tft.print(" W");
  tft.setCursor(5, 190); tft.print("SWR");
  tft.setCursor(80, 190); tft.print(swr);
  tft.setFreeFont(NULL);
}
#endif

// BEWARE ! If you activate WiFi, the ADC2 (GPIO4, GPIO0, GPIO2, GPIO15, GPIO13, GPIO12, GPIO14, GPIO27, GPIO25 and GPIO26) is blocked and can't be used !!!
// only ADC1 (GPIO36, GPIO37, GPIO38, GPIO39, GPIO32, GPIO33, GPIO34 and GPIO35) can be used together with WiFi !

// define ADC input PINs for T-DISPLAY, don't forget to connect GND XPA<->ESP32 in addition !
const int PoutPin = 32; // ADC forward power from XPA125B
const int PrefPin = 33; // ADC reflected power from XPA125B

// here we define the HTML startpage with CSS and JS, all will be saved only in memory of ESP32
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 DASHBOARD</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
html {
  font-family: Arial, Helvetica, sans-serif;
  display: inline-block;
  text-align: center;
}
h1 {
  font-size: 1.8rem;
  color: white;
}
.topnav {
  overflow: hidden;
  background-color: #0A1128;
}
body {
  margin: 0;
}
.content {
  padding: 50px;
}
.card-grid {
  max-width: 800px;
  margin: 0 auto;
  display: grid;
  grid-gap: 2rem;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
}
.card {
  background-color: white;
  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
}
.card-title {
  font-size: 1.2rem;
  font-weight: bold;
  color: #034078
}
.reading {
  font-size: 1.2rem;
  color: #1282A2;
}
</style>
</head>
<body>
        <div class="topnav">
            <h1>XPA125B Status</h1>
        </div>
        <div class="content">
            <div class="card-grid">
                <div class="card">
                    <p class="card-title"> P<sub>forward</sub></p>
                    <p class="reading"><span id="pwrfwd"></span> W</p>
                </div>
                <div class="card">
                    <p class="card-title"> P<sub>reflected</sub></p>
                    <p class="reading"><span id="pwrref"></span> W</p>
                </div>
                <div class="card">
                    <p class="card-title"> SWR</p>
                    <p class="reading"><span id="swr"></span></p>
                </div>
            </div>
        </div>
<script>
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
}
</script>
</body>
</html>
)rawliteral";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// read from ADC, parameter is input PIN ADC ESP32
int Read_PWR(int PIN) {
  int ADCval = 0;               // ADC values between 0 and 4095
  int mV = 0;                   // recalculate ADC value to mV
  float corrFact = 0.12;        // we are not 100% linear, so we need a kind of correction value
  ADCval = analogRead(PIN);     // read direct from ADC
  // we continue only if ADC value > 0
  if (ADCval > 0) {
    mV = ADCval*1250/4096;
    if (mV >=720) {
      corrFact = 0.129;
    }
    if (mV < 720) {
      corrFact = 0.129;
    }
    if (mV < 630 ) {
      corrFact = 0.126;
    }
    if (mV < 400) {
      corrFact = 0.08;
    }
    return (mV*corrFact);
    } else {
    // otherwise we send Zero return
    return ADCval;
  }
}

// read from ADC, parameter is input PIN ADC ESP32
// here we running 16 measurements and calculate an average value for
// reduce the ADC noise
int Read_PWR_avg(int PIN) {
  int i;                       // loop counter
  int j = 16;
  int ADCval[2];               // ADC array with values between 0 and 4095
  int ADCavg = 0;              // ADC average
  int mV = 0;                  // recalculate ADC value to mV
  float corrFact = 0.12;       // we are not 100% linear, so we need a kind of correction value
  for (i = 0; i <= 15; i++) {
    ADCval[i] = analogRead(PIN); // read direct from ADC
    usleep(1000); // in micro seconds 1000us = 1ms
      if ((ADCval[i] == 0) && (j > 1)) { j=j-1; } // eleminate values with 0 from chain
    ADCavg += ADCval[i]; // sum of all values
  }
    Serial.print("Messungen: "); Serial.println(j);
    ADCavg = ADCavg/j; // generate the average of all
   
  // we continue only if ADC value > 0
  if (ADCavg > 0) {
    mV = ADCavg*1250/4096; // 12bit and 2.5db attenuation = min. 100mV and max. voltage 1.250V at ADC input
    Serial.print("Spannung (mV) :"); Serial.println(mV);
    if (mV >=720) {
      corrFact = 0.129;
    }
    if (mV < 720) {
      corrFact = 0.129;
    }
    if (mV < 630 ) {
      corrFact = 0.126;
    }
    if (mV < 400) {
      corrFact = 0.08;
    }
    return (mV*corrFact); // return PA power in W
  } else {
    // otherwise we send Zero return
    return ADCavg;
  }
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  float pwrfwd = Read_PWR_avg(PoutPin); // read ADC for forward power
  float pwrref = Read_PWR(PrefPin);     // read ADC for reflected power
  float swr;
  if (pwrfwd > 0) { swr = (pwrfwd+pwrref)/(pwrfwd-pwrref); } else { swr = 0; } // calculate SWR with formula U(f)+U(r)/U(f)-U(r)
  readings["pwrfwd"] = String(pwrfwd,0);
  readings["pwrref"] = String(pwrref,0);
  readings["swr"] = String(swr,1);
  #ifdef DISPLAY
  showDATA_TFT(readings["pwrfwd"],readings["pwrref"],readings["swr"]);
  #endif
  String jsonString = JSON.stringify(readings); // convert answer to JSON format
  return jsonString;
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 500; // in ms, 1000 = 1s

// Replace with your network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void init_UARTs() {
  Serial.begin(115200);                        // initialize internal UART with 115200/8N1
}

void init_ADCs() {
  adcAttachPin(PoutPin);                       // ADC input at PINxx
  adcAttachPin(PrefPin);                       // ADC input at PINxx
  analogSetWidth(12);                          // 12bit resolution
  // ESP32 ADC ranges depends of selected attenuation, default is 11db
  //   0db 100mV ~  950mV
  // 2.5db 100mV ~ 1250mV
  //   6db 150mV ~ 1750mV
  //  11db 150mV ~ 3100mV
  analogSetPinAttenuation(PoutPin, ADC_2_5db); // select ATT 2.5db
  analogSetPinAttenuation(PrefPin, ADC_2_5db); // select ATT 2.5db
}

void init_WLAN() {
  WiFi.setHostname("ESP32-XPA"); // set the hostname if needed
  WiFi.setAutoReconnect(true);   // auto-reconnect WiFi if connection is interrupted
  WiFi.mode(WIFI_STA);           // Mode Station, not AP
  WiFi.begin(ssid, password);    // Connect WiFi with your crendentials
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  // info output IP and hostname
  Serial.println();
  Serial.print("Hostname: "); Serial.println(WiFi.getHostname());
  Serial.print("IP      : "); Serial.println(WiFi.localIP());
}

void setup() {
  // put your setup code here, to run once:

// init the needed things
init_UARTs(); Serial.println(VERSION); // init internal UART for debug output
init_WLAN();                           // init WiFi
init_ADCs();                           // init ADC

#ifdef TDISPLAY
init_TFT();                            // initialize T-DISPLAY LILYGO TTGO v1.1
if (WiFi.status() == WL_CONNECTED) { statusWIFI_TFT(); } // show WiFi status infos at TFT
#endif

initWebSocket();                       // init websocket

// define webserver root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start webserver
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if ((millis() - lastTime) > timerDelay) {
  String sensorReadings = getSensorReadings();
    Serial.println(sensorReadings); // debug output
    notifyClients(sensorReadings);  // update webpage in realtime with websockets
  lastTime = millis();
}
  ws.cleanupClients();
}
