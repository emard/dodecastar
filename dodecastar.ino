#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 2
#define NUM_LEDS 50
// web interface is stable up to 5 LEDs
// for more, web interface will stop working after few clicks
// workaround: in Adafruit_NeoPixel.cpp around line 149
// comment out "noInterrupts()" call
// /*  noInterrupts(); */
// LEDs will flicker with interrupts ON, but web interface should be stable
// Interrupts can be turned ON/OFF with "FLICKER" web button

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// remote updates over the air
// (>1M flash required)
#define USE_OTA 0

// includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <FS.h>
#if USE_OTA
#include <ArduinoOTA.h>
#endif
#include <ESP8266WebServer.h>

/**
 * mDNS and OTA Constants
 */
#define HOSTNAME "dodecastar-" ///< Hostename. The setup function adds the Chip ID at the end.
/// @}

#define COUNTDOWN_CYCLES 10
int countdown = COUNTDOWN_CYCLES; // improve web stability after clicking, do some loop() cycles without ledstrip update
int allow_interrupts = 1; // no interrupts: web stability only with no interrupts, interrupts: noflicker
int program_control = 0; // blink leds using program control
int program_speed = 5; // how fast
int program_density = 2; // how dense does the color changes between pixels
int program_brightness = 50; // intensity of the LEDs

/**
 * Default WiFi connection information.
 */
const char* ap_default_ssid = "ra"; ///< Default SSID.
const char* ap_default_psk = "GigabyteBrix"; ///< Default PSK.
//const char* ap_default_psk = ""; ///< Default PSK.
const char* config_name = "./dodecastar.conf";

String current_ssid = ap_default_ssid;
String current_psk = ap_default_psk;

String message = "";
ESP8266WebServer server(80);
String webString="";     // String to display (runtime modified)
// too many controls will make web interface unstable
// enable checkbox for leds
#define WEB_TAB_CHECKBOX 0
// enable on-off buttons for leds
#define WEB_TAB_BUTTONS 0

int ssr_cols = 5, ssr_rows = NUM_LEDS/5; // ssr display shown as table cols x rows

uint32_t led_color[NUM_LEDS];
uint8_t led_on[NUM_LEDS]; // click for on/off

/**
 * Read WiFi connection information from file system.
 * ssid String pointer for storing SSID.
 * pass String pointer for storing PSK.
 * return True or False.
 * 
 * The config file have to containt the WiFi SSID in the first line
 * and the WiFi PSK in the second line.
 * Line seperator can be \r\n (CR LF) \r or \n.
 */
bool loadConfig(String *ssid, String *pass)
{
  // open file for reading.
  File configFile = SPIFFS.open(config_name, "r");
  if (!configFile)
  {
    Serial.print("Failed to load ");
    Serial.println(config_name);
    return false;
  }

  // Read content from config file.
  String content = configFile.readString();
  configFile.close();
  
  content.trim();

  // Check if there is a second line available.
  int8_t pos = content.indexOf("\r\n");
  uint8_t le = 2;
  // check for linux and mac line ending.
  if (pos == -1)
  {
    le = 1;
    pos = content.indexOf("\n");
    if (pos == -1)
      pos = content.indexOf("\r");
  }

  // If there is no second line: Some information is missing.
  if (pos == -1)
  {
    Serial.println("Infvalid content.");
    Serial.println(content);
    return false;
  }

  // check for the third line
  // Check if there is a second line available.
  int8_t pos2 = content.indexOf("\r\n", pos + le + 1);
  uint8_t le2 = 2;
  // check for linux and mac line ending.
  if (pos2 == -1)
  {
    le2 = 1;
    pos2 = content.indexOf("\n", pos + le + 1);
    if (pos2 == -1)
      pos2 = content.indexOf("\r", pos + le + 1);
  }
  
  // If there is no third line: Some information is missing.
  if (pos2 == -1)
  {
    Serial.println("Invalid content.");
    Serial.println(content);
    return false;
  }

  // Store SSID and PSK into string vars.
  *ssid = content.substring(0, pos);
  *pass = content.substring(pos + le, pos2);

  // get LED state
  #if 0
  String ssr_state = content.substring(pos2 + le2);
  for(int i = 0; i < ssr_state.length() && i < NUM_LEDS; i++)
    led_color[i] = (ssr_state.substring(i,i+1) == "1" ? 1 : 0);
  output_state();
  #endif
  ssid->trim();
  pass->trim();

#ifdef SERIAL_VERBOSE
  Serial.println("----- file content -----");
  Serial.println(content);
  Serial.println("----- file content -----");
  Serial.println("ssid: " + *ssid);
  Serial.println("psk:  " + *pass);
  Serial.println("ssr:  " +  ssr_state);
#endif

  return true;
} // loadConfig


/**
 * @brief Save WiFi SSID and PSK to configuration file.
 * @param ssid SSID as string pointer.
 * @param pass PSK as string pointer,
 * @return True or False.
 */
bool saveConfig(String *ssid, String *pass)
{
  // Open config file for writing.
  File configFile = SPIFFS.open(config_name, "w");
  if (!configFile)
  {
    Serial.print("Failed to save ");
    Serial.println(config_name);
    return false;
  }

  // Save SSID and PSK.
  configFile.println(*ssid);
  configFile.println(*pass);

  configFile.close();
  
  return true;
} // saveConfig

// format filesystem (erase everything)
// place default password file
void format_filesystem(void)
{
  String station_ssid = ap_default_ssid;
  String station_psk = ap_default_psk;

  Serial.println("Formatting"); // erase everything
  SPIFFS.format();
  
  Serial.println("Saving factory default");
  saveConfig(&station_ssid, &station_psk);
}

// output led state to hardware pins
void output_state()
{
  for(int i = 0; i < strip.numPixels() && i < NUM_LEDS; i++)
    strip.setPixelColor(i, led_color[i]);
}


// greate html table
// that displays ssr state
// in color and has submit buttons
void create_message()
{
  int n = 0;
  message =
"<style type=\"text/css\">"
"input[type=checkbox]"
"{"
 "/* Double-sized Checkboxes */"
 "-ms-transform: scale(2); /* IE */"
 "-moz-transform: scale(2); /* FF */"
 "-webkit-transform: scale(2); /* Safari and Chrome */"
 "-o-transform: scale(2); /* Opera */"
 "padding: 10px;"
"}"
"</style>"
"<head>"
"<meta http-equiv=\"cache-control\" content=\"max-age=0\" />"
"<meta http-equiv=\"cache-control\" content=\"no-cache\" />"
"<meta http-equiv=\"expires\" content=\"0\" />"
"<meta http-equiv=\"expires\" content=\"Tue, 01 Jan 1980 1:00:00 GMT\" />"
"<meta http-equiv=\"pragma\" content=\"no-cache\" />"
"</head>"
            "<a href=\"/\">refresh</a> "
            "<a href=\"setup\">setup</a> "
            "<a href=\"cssButton\">cssButton</a> "
            "<p/>"
            "<form action=\"/update\" method=\"get\" autocomplete=\"off\">";
  // this controls each individual LED on the strip
  message += "<table>";
  for(int y = 0; y < ssr_rows; y++)
  {
    message += "<tr>";
    for(int x = 0; x < ssr_cols; x++)
    {
      char hexcolor[10];
      sprintf(hexcolor, "#%06X", led_color[n]);
      message += "<td bgcolor=\"" + String(hexcolor) + "\">"
#if WEB_TAB_CHECKBOX
               + "<input type=\"checkbox\" name=\"check" + String(n) + "\" " + String(led_on[n] ? " checked" : "") + "> </input>"
#endif
               + "<input type=\"color\" name=\"color" + String(n) + "\" value=\"" + String(hexcolor) + "\"> </input>"
#if WEB_TAB_BUTTONS
               + "<button type=\"submit\" name=\"button"
               + String(n) 
               + "\" value=\"" 
               + String(led_on[n] ? "0" : "1") 
               + "\">" // toggle when clicked 
               + String(led_on[n] ? "ON" : "OFF") // current state
               + "</button>"
#endif
                 "</td>";
      n++; // increment ssr number
    }
    message += "</tr>";
  }
  message += "</table>";
  // interrupts on/off
  message +=     "FLICKER "
                 " <button type=\"submit\" name=\"interrupt_btn\" value=\"" 
               + String(allow_interrupts ? "0" : "1") 
               + "\">" // toggle when clicked 
               + String(allow_interrupts ? "ON" : "OFF") // current state
               + "</button>"
               + String(allow_interrupts ? "" : " if LED strip stops responding, power OFF/ON")
               + "<p/>";
  // program control on/off
  message +=     "PROGRAM "
                 " <button type=\"submit\" name=\"program_btn\" value=\"" 
               + String(program_control ? "0" : "1") 
               + "\">" // toggle when clicked 
               + String(program_control ? "ON" : "OFF") // current state
               + "</button>"
                 "<p/>";
  // program speed
  message +=     "SPEED "
               " <input type=\"number\" name=\"speed\" value=\"" 
               + String(program_speed) 
               + "\" min=\"1\" max=\"10\">"
                 "<p/>";
  // program density
  message +=     "DENSITY "
               " <input type=\"number\" name=\"density\" value=\"" 
               + String(program_density)
               + "\" min=\"1\" max=\"10\">"
                 "<p/>";
  // program brightness
  message +=     "BRIGHTNESS "
               " <input type=\"number\" name=\"brightness\" value=\"" 
               + String(program_brightness)
               + "\" min=\"0\" max=\"256\">"
                 "<p/>";

  message += "<button type=\"submit\" name=\"apply\" value=\"1\">Apply</button>"
             "<button type=\"submit\" name=\"save\" value=\"1\">Save</button>";
 
  message += "</form>";
}

// when user requests root web page
void handle_root()
{
  create_message();
  server.send(200, "text/html", message);
  countdown = COUNTDOWN_CYCLES;
}

void handle_setup() {
  String new_ssid = "", new_psk = "";
  webString = "<form action=\"/setup\" method=\"get\" autocomplete=\"off\">"
              "Access point: <input type=\"text\" name=\"ssid\"><br>"
              "Password: <input type=\"text\" name=\"psk\"><br>"
              "<button type=\"submit\" name=\"apply\" value=\"1\">Apply</button>"
              "<button type=\"submit\" name=\"discard\" value=\"1\">Discard</button>"
              "</form>";
  for(int i = 0; i < server.args(); i++)
  {
    if(server.argName(i) == "discard")
    {
      loadConfig(&current_ssid, &current_psk);
      create_message();
      webString = message;
    }
    if(server.argName(i) == "apply")
    {
      for(int j = 0; j < server.args(); j++)
      {
        if(server.argName(j) == "ssid")
        {
          new_ssid = server.arg(j);
        }
        if(server.argName(j) == "psk")
        {
          new_psk = server.arg(j);
        }
      }
      if(new_ssid.length() > 0 && new_psk.length() >= 8)
      {
        //Serial.println("Save config");
        current_ssid = new_ssid;
        current_psk = new_psk;
        //saveConfig(&current_ssid, &current_psk);
        //reboot = 1;
        //loadConfig(&current_ssid, &current_psk);
        create_message();
        webString = message 
          + String("Click Save for new login:<p/>")
          + "Access point: " + current_ssid + "<br/>"
          + "Password: " + current_psk + "<p/>"
          + "Settings will be active after next power up.";
      }
    }
  }
  server.send(200, "text/html", webString);            // send to someones browser when asked
  countdown = COUNTDOWN_CYCLES;
}

void handle_update() {
  // Apply or Save button
  for(int i = 0; i < server.args(); i++)
  {
    if(server.argName(i) == "apply" || server.argName(i) == "save")
    {
      // assume all are off
      for(int j = 0; j < NUM_LEDS; j++)
        led_on[j] = 0;
      // checkboxes on
      for(int j = 0; j < server.args(); j++)
      {
        #if WEB_TAB_CHECKBOX
        if(server.argName(j).startsWith("check"))
        {
          int n = server.argName(j).substring(5).toInt();
          if(n >= 0 && n < NUM_LEDS)
            if(server.arg(j) == "on")
              led_on[n] = 1;
        }
        #endif
        if(server.argName(j).startsWith("color"))
        {
          int n = server.argName(j).substring(5).toInt();
          if(n >= 0 && n < NUM_LEDS)
          {
            char hexcolor[10];
            server.arg(j).substring(1).toCharArray(hexcolor, 7);
            uint32_t bincolor = strtol(hexcolor, NULL, 16);
            led_color[n] = strip.Color((bincolor >> 16) & 255, (bincolor >> 8) & 255, bincolor & 255);
          }
        }
      }
    }
    if(server.argName(i) == "save")
      saveConfig(&current_ssid, &current_psk);
  }
  // ON/OFF buttons
  for(int i = 0; i < server.args(); i++)
  {
    #if WEB_TAB_BUTTONS
    if(server.argName(i).startsWith("button"))
    {
      int n = server.argName(i).substring(6).toInt();
      if(n >= 0 && n < NUM_LEDS)
        led_on[n] = server.arg(i).toInt() ? 1 : 0;
    }
    #endif
    if(server.argName(i).startsWith("interrupt_btn"))
      allow_interrupts = server.arg(i).toInt();
    if(server.argName(i).startsWith("program_btn"))
      program_control = server.arg(i).toInt();
    if(server.argName(i).startsWith("speed"))
      program_speed = server.arg(i).toInt();
    if(server.argName(i).startsWith("density"))
      program_density = server.arg(i).toInt();
    if(server.argName(i).startsWith("brightness"))
      program_brightness = server.arg(i).toInt();
  }
  if(program_control == 0)
    output_state();
  create_message();
  webString = message;
  #if 0
  // some debugging print post/get messages
  webString += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  for (int i = 0; i < server.args(); i++ )
  {
    webString += " " + server.argName(i) + ": " + server.arg(i);
  };
  #endif
  server.send(200, "text/html", webString);
  countdown = COUNTDOWN_CYCLES;
}

// fancy css color button example
void handle_cssButton()
{
  const char * cssButton ="<!DOCTYPE html>"
"<html>"
"<head>"
"<style>"
".button {"
"background-color: #990033;"
"border: none;"
"color: white;"
"padding: 7px 15px;"
"text-align: center;"
"cursor: pointer;"
"}"
"</style>"
"</head>"
"<body>"
"<input type=\"button\" class=\"button\" value=\"Input Button\">"
"</body>"
"</html>";

  server.send(200, "text/html", cssButton);
}

void program()
{
  uint32_t t = millis() >> (10-program_speed); // time
  static uint32_t old_t = 0;
  static uint8_t p = 0;
  
  if(t != old_t)
  {
    old_t = t;
    for(int i = 0; i < NUM_LEDS; i++)
    {
      strip.setPixelColor(i, Wheel_bright( ( (i<<(program_density-1)) - t ) & 255, program_brightness) );
    }
  }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  String station_ssid = "";
  String station_psk = "";

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // "LOW" will turn LED on

  Serial.begin(115200);
  delay(100);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);

  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  // Print hostname.
  Serial.println("Hostname: " + hostname);
  //Serial.println(WiFi.hostname());

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  // Load wifi connection information.
  #if 0
  if (! loadConfig(&station_ssid, &station_psk))
  {
    station_ssid = "";
    station_psk = "";

    Serial.println("No WiFi connection information available.");
    format_filesystem();
    Serial.println("Trying again");
    
    if (! loadConfig(&station_ssid, &station_psk))
    {
      station_ssid = "";
      station_psk = "";

      Serial.println("Second time failed. Cannot create filesystem.");
    }
  }ž
  #else
    station_ssid = ap_default_ssid;
    station_psk = ap_default_psk;
  #endif

  // ... Compare file config with sdk config.
  if (WiFi.SSID() != station_ssid || WiFi.psk() != station_psk)
  {
    Serial.println("WiFi config changed.");

    // ... Try to connect to WiFi station.
    WiFi.begin(station_ssid.c_str(), station_psk.c_str());

    // ... Pritn new SSID
    Serial.print("new SSID: ");
    Serial.println(WiFi.SSID());

    // ... Uncomment this for debugging output.
    //WiFi.printDiag(Serial);
  }
  else
  {
    // ... Begin with sdk config.
    WiFi.begin();
  }

  Serial.println("Wait for WiFi connection.");

  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
  {
    Serial.write('.');
    //Serial.print(WiFi.status());
    delay(500);
  }
  Serial.println();

  // Check connection
  if(WiFi.status() == WL_CONNECTED)
  {
    // ... print IP Address
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    colorWipe(strip.Color( 0,  50,   0), 40); // Dim Green init
  }
  else
  {
    Serial.println("No connection to remote AP. Becoming AP itself.");
    Serial.println(ap_default_ssid);
    Serial.println(ap_default_psk);
    // Go into software AP mode.
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP(ap_default_ssid, ap_default_psk);

    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    colorWipe(strip.Color(50,   0,   0), 40); // Dim Red init
  }
  current_ssid = station_ssid;
  current_psk = station_psk;

#if USE_OTA
  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
#endif

  server.on("/", handle_root);
  //server.on("/read", handle_read);
  server.on("/setup", handle_setup);
  server.on("/update", handle_update);
  server.on("/cssButton", handle_cssButton);
  create_message();
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  #if USE_OTA
  // Handle OTA server.
  ArduinoOTA.handle();
  #endif
  // digitalWrite(LED_BUILTIN, HIGH); // LED OFF
  // Handle web server
  server.handleClient();
  if(countdown > 0)
    countdown--;
  else
  {
    if(program_control > 0)
      program();
    if(allow_interrupts == 0)
      noInterrupts();
    strip.show(); // it will disable interrupts
    interrupts(); // re-enable interrupts
  }
#if 0
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255,   0,   0), 40); // Red
  colorWipe(strip.Color(255, 127,   0), 40); // Orange
  colorWipe(strip.Color(  0, 255,   0), 40); // Green
  colorWipe(strip.Color(  0, 127, 127), 40); // Cyan
  colorWipe(strip.Color(  0,   0, 255), 40); // Blue
  colorWipe(strip.Color(255,   0, 127), 40); // Violett
  colorWipe(strip.Color(  0,   0,   0), 40); // Black
  //colorWipe(strip.Color(0, 0, 0, 255), 40); // White RGBW
  //delay(1000);
  for(int i = 0; i < 3; i++)
    rainbow(20);
  // Send a theater pixel chase in...
  theaterChase(strip.Color(255, 127,   0), 16, 2, 20); // Orange
  theaterChase(strip.Color(255,   0, 127), 16, 2, 20); // Violett
  theaterChase(strip.Color(127, 127, 127), 16, 2, 20); // White
  rainbowCycle(20);
  theaterChaseRainbow(32, 20);
#endif
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t every, uint8_t n, uint8_t wait) {
  for (int j=0; j<n; j++) {  //do n cycles of chasing
    for (int q=0; q < every; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+every) {
        strip.setPixelColor(i+q, c);    //turn every nth pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+every) {
        strip.setPixelColor(i+q, 0);        //turn every nth pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t every, uint8_t wait) {
  for (int j=0; j < 256; j+=16) {     // cycle all 256 colors in the wheel
    for (int q=0; q < every; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+every) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+every) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t Wheel_bright(byte WheelPos, byte brightness) {
  uint32_t r,g,b;
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    r = 255 - WheelPos * 3;
    g = 0;
    b = WheelPos * 3;
  } else
  if(WheelPos < 170) {
    WheelPos -= 85;
    r = 0;
    g = WheelPos * 3;
    b = 255 - WheelPos * 3;
  } else
  {
    WheelPos -= 170;
    r = WheelPos * 3;
    g = 255 - WheelPos * 3;
    b = 0;
  }
  r = r * brightness / 256;
  g = g * brightness / 256;
  b = b * brightness / 256;
  return strip.Color(r, g, b);
}

