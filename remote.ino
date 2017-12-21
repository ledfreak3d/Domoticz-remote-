//////// Source code bij Martijn vos ///////////////


#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "ESP8266FtpServer.h"
#include <Ticker.h>
Ticker ticker;

unsigned long interval = 300000; // 300000 = 5 min interval
unsigned long previousMillis = 0; // millis() returns an unsigned long.
unsigned int raw = 0;
float volt = 0.0;   // set initial volt value
String v = String(volt); // change float into string
//int old_sensor_value = 0;
const int analogInPin = A0;
int sensorValue = 0;        // value read from analog input
int output_value = 0;        // value output to display
const char* host = "Domoremote";  // just the host name that shows up on the network
int value = 0;
String tmpidx = "47";   // stores temporary idx numbers

///  Set your default servers settings ////
char dom_server[15] = "192.168.1.3";
char custom_dom_server[15];   // holds value recieved from web site config for saving to json file
int httpport = 8084;  // Domoticz port
char dom_port[6] = "8084";

////Dimmer IDX numbers ////////
char dom_idx[6] = "377";
char dim_idx1[6] = "0";
char dim_idx2[6] = "0";
char dim_idx3[6] = "0";
char dim_idx4[6] = "0";
char dim_idx5[6] = "0";
char dim_idx6[6] = "0";
char dim_idx7[6] = "0";
char dim_idx8[6] = "0";
char dim_idx9[6] = "0";
char dim_idx10[6] = "0";

//////Switch idx ////////////////

char sw_idx1[6] = "0";
char sw_idx2[6] = "0";
char sw_idx3[6] = "0";
char sw_idx4[6] = "0";
char sw_idx5[6] = "0";
char sw_idx6[6] = "0";
char sw_idx7[6] = "0";
char sw_idx8[6] = "0";
char sw_idx9[6] = "0";
char sw_idx10[6] = "0";

/////////////////////Wifi Vars ////////////////////////////////
String ssid;  //string that holds ssid
String pass;  // string that holds the password
String rssi;  // string that holds the signal strenght
String ip2;   //Client ip addres
String apssid;  // Acces point ip addres

//////////////////////////Website vars //////////////////////
String siteheading   = "Domoticz remote web interface";
String subheading    = "Domoremote setup and info";
String sitetitle     = "Domoremote"; // Appears on browser tabs
String yourcopyright = "Ledfreak3d";
String siteversion   = "v1.12"; // Version of your webserver
String webpage       = "";     // General purpose variable to hold HTML code
#define sitewidth 1024         // Adjust site page width as required
String page1_txt = "Dimmer setup"; // Adjust the text to suit your needs, for example page1txt = 'Temperature'
String page2_txt = "Switch setup";
bool   page1_enabled = false; // Set to false to dimove/disable each command as required
bool   page2_enabled = false;
////////////////Server init ////////////////////////////////
ESP8266WebServer webserver(80);
FtpServer ftp;
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

/////////////// Display Init //////////////////
SoftwareSerial nextion(5, 4);   // set rx and tx to  pins 5 and 4
Nextion myNextion(nextion, 9600);//create a Nextion object named myNextion using the nextion serial port @ 9600bps

void configModeCallback (WiFiManager *myWiFiManager) {
  WiFiManager wifiManager;
  Serial.println("Entered config mode");
  ip2 = WiFi.softAPIP().toString();
  apssid = wifiManager.getConfigPortalSSID();
  Serial.println(WiFi.softAPIP());
  myNextion.sendCommand("page wifi");  // goto nexion wifi page
  myNextion.setComponentText("t15", "Please connect to network:");
  myNextion.setComponentText("t18", "And browse to :");
  myNextion.setComponentText("t17", ip2);
  myNextion.setComponentText("t16", apssid);
  ticker.attach(0.2, tick);
  //if you used auto generated SSID, print it
  Serial.println(apssid);

}
void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  myNextion.init();
  pinMode(A0, INPUT);
  int sensorValue = 0;
  ssid = WiFi.SSID();
  const int analogRead(A0);
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("Domo-remote", "password")) {
    ticker.attach(0.8, tick);
    Serial.println("failed to connect and hit timeout");
    delay(1000);
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected to :");
  Serial.print(ssid);
  Serial.println(" ");
  String ipaddress = WiFi.localIP().toString();
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  webserver.begin();
  MDNS.addService("http", "tcp", 80);
  //String ipaddress = WiFi.localIP().toString();
  Serial.println(F("----Webserver started----"));
  Serial.println("Use this URL to connect: http://" + ipaddress + "/"); // Print the IP address
  webserver.on("/",         setup_input); // If the user types at their browser http://192.168.0.100/ control is passed here and then to user_input, you get values for your program...
  webserver.on("/homepage", homepage);   // If the user types at their browser http://192.168.0.100/homepage or via menu control is passed here and then to the homepage
  webserver.on("/command1", page1);      // If the user types at their browser http://192.168.0.100/page1 or via menu control is passed here and then to the page1
 



  // String rssi = WiFi.RSSI().toString();
  // Serial.println(WiFi.RSSI());
  myNextion.sendCommand("page home");   // goto home page on display
  myNextion.setComponentText("t1", String(ssid));
  myNextion.setComponentText("t4", String(ipaddress));
  myNextion.sendCommand("t10.val,10" );
  myNextion.setComponentValue("j2", output_value);
  myNextion.sendCommand("va0.val,1");
  myNextion.sendCommand("vis p0,1");

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json

  if (SPIFFS.begin()) {
    Serial.println("===  mnt file system  ===");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("== reading config file ==");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        //Serial.println("== opened cfg ==");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {

          strcpy(dom_server, json["dom_server"]);
          strcpy(dom_port, json["dom_port"]);
          strcpy(dom_idx, json["dom_idx"]);
          strcpy(dim_idx1, json["dim_idx1"]);
          strcpy(dim_idx2, json["dim_idx2"]);
          strcpy(dim_idx3, json["dim_idx3"]);
          strcpy(dim_idx4, json["dim_idx4"]);
          strcpy(dim_idx5, json["dim_idx5"]);
          strcpy(dim_idx6, json["dim_idx6"]);
          strcpy(dim_idx7, json["dim_idx7"]);
          strcpy(dim_idx8, json["dim_idx8"]);
          strcpy(dim_idx9, json["dim_idx9"]);
          strcpy(dim_idx10, json["dim_idx10"]);
          strcpy(sw_idx1, json["sw_idx1"]);
          strcpy(sw_idx2, json["sw_idx2"]);
          strcpy(sw_idx3, json["sw_idx3"]);
          strcpy(sw_idx4, json["sw_idx4"]);
          strcpy(sw_idx5, json["sw_idx5"]);
          strcpy(sw_idx6, json["sw_idx6"]);
          strcpy(sw_idx7, json["sw_idx7"]);
          strcpy(sw_idx8, json["sw_idx8"]);
          strcpy(sw_idx9, json["sw_idx9"]);
          strcpy(sw_idx10, json["sw_idx10"]);
          Serial.println("== Config loaded ==");

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  httpport = atoi(dom_port);

  if (SPIFFS.begin()) {
    Serial.println(F("----Ftpserver started----"));
    ftp.begin("admin", "6077"); // username, password for ftp. Set ports in ESP8266FtpServer.h (default 21, 50009 for PASV)
  }
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality(20);

}

void SaveConfig() {
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["dom_server"] = dom_server;
  json["dom_port"] = dom_port;
  json["dom_idx"] = dom_idx;
  json["dim_idx1"] = dim_idx1;
  json["dim_idx2"] = dim_idx2;
  json["dim_idx3"] = dim_idx3;
  json["dim_idx4"] = dim_idx4;
  json["dim_idx5"] = dim_idx5;
  json["dim_idx6"] = dim_idx6;
  json["dim_idx7"] = dim_idx7;
  json["dim_idx8"] = dim_idx8;
  json["dim_idx9"] = dim_idx9;
  json["dim_idx10"] = dim_idx10;
  json["sw_idx1"] = sw_idx1;
  json["sw_idx2"] = sw_idx2;
  json["sw_idx3"] = sw_idx3;
  json["sw_idx4"] = sw_idx4;
  json["sw_idx5"] = sw_idx5;
  json["sw_idx6"] = sw_idx6;
  json["sw_idx7"] = sw_idx7;
  json["sw_idx8"] = sw_idx8;
  json["sw_idx9"] = sw_idx9;
  json["sw_idx10"] = sw_idx10;
  ;


  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();

  //end save

}

void homepage() {


  append_HTML_header();
  webpage += "<P class='style5'>Domoremote home page</p>";
  // webpage += "<p class='style5'>";
  webpage += "Welcome to the Domoremote webinterface.";
  webpage += "Here you can setup and view information regarding your remote.";
  webpage += "Please goto the setting page to change your idx settings and wifi setup";
  webpage += "</p>";
  append_HTML_footer();
  webserver.send(200, "text/html", webpage);

  MDNS.begin(host);
  webserver.on("/update", HTTP_GET, []() {
    webserver.sendHeader("Connection", "close");
    webserver.sendHeader("Access-Control-Allow-Origin", "*");
    webserver.send(200, "text/html", serverIndex);
  });
  webserver.on("/update", HTTP_POST, []() {
    webserver.sendHeader("Connection", "close");
    webserver.sendHeader("Access-Control-Allow-Origin", "*");
    webserver.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.reset();
  }, []() {
    HTTPUpload& upload = webserver.upload();

    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
      myNextion.setComponentText("b50", "FW:Update OK !");
    }
    yield();
  });

}

void page1() {
  append_HTML_header();
  //  webpage += "<H3> Page-1</H3>";
  webpage += "<P class='style5'>This is the server home page</p>";
  webpage += "<p class='style5'>";
  webpage += "";
  webpage += "</p>";
  append_HTML_footer();
  webserver.send(200, "text/html", webpage);
}


void append_HTML_header() {
  webpage = "HTTP/1.1 200 OK";
  webpage += "Content-Type: text/html";
  webpage += "Connection: close";
  //  webpage += "Refresh: 30";
  webpage += "<!DOCTYPE html><html><head>";
  //webpage += "<meta http-equiv='refresh' content='60'>"; // 30-sec refresh time, test needed to prevent auto updates repeating some commands
  webpage += "<title>" + sitetitle + "</title><style>";
  webpage += "body {width:" + String(sitewidth) + "px;margin:0 auto;font-family:arial;font-size:14px;text-align:center;color:blue;background-color:#F7F2Fd;}";
  webpage += "</style></head><body><h1>" + siteheading + " " + siteversion + "</h1>";
  webpage += "<h3>" + subheading + "<br></h1>";
}

void append_HTML_footer() { // Saves repeating many lines of code for HTML page footers
  webpage += "<head><style>ul{list-style-type:none;margin:0;padding:0;overflow:hidden;background-color:#d8d8d8;font-size:14px;}";
  webpage += "li{float:left;border-right:1px solid #bbb;}last-child {border-right: none;}";
  webpage += "li a{display: block;padding:3px 15px;text-decoration:none;}";
  webpage += "li a:hover{background-color:#FFFFFF;}";
  webpage += "section {font-size:14px;}";
  webpage += "p {background-color:#787578;}";  //  was#E3D1E2
  webpage += "h1{background-color:#d8d8d8;}";
  webpage += "h3{color:#9370DB;font-size:24px;}";
  webpage += "table{font-family: arial, sans-serif;font-size:16px;border-collapse: collapse;width: 100%;}";
  webpage += "td, th {border: 0px solid black;text-align: left;padding: 2px;}";
  webpage += ".style1 {text-align:center;font-size:24px; background-color:#D8BFD8;}";
  webpage += ".style2 {text-align:center;font-size:20px; background-color:#ADD8E6;}";
  webpage += ".style3 {text-align:center;font-size:16px; background-color:#FFE4B5;}";
  webpage += ".style4 {text-align:center;font-size:14px; background-color:#FFE4B5;}";
  webpage += ".style5 {text-align:center;font-size:14px; background-color:#D9BFD9;}";
  webpage += ".style6 {text-align:left;  font-size:16px; background-color:#F7F2Fd; width:100%;}";
  webpage += "sup{vertical-align:super;font-size:smaller;}"; // superscript controls
  webpage += "</style>";
  webpage += "<ul>";
  webpage += "  <li><a href='/homepage'>Home Page</a></li>";
  if (page1_enabled) webpage += "  <li><a href='/dimmer'>" + page1_txt + "</a></li>";
  if (page2_enabled) webpage += "  <li><a href='/switch'>" + page2_txt + "</a></li>";
  webpage += "  <li><a href='/setup_input'>Setup</a></li>";
  webpage += "  <li><a href='/update'>Fw-Update</a></li>";
  webpage += "</ul>";
  webpage += "<p>&copy; " + yourcopyright + " 2017<br>";
  webpage += "</body></html>";
}

void setup_input() {

  //

  webpage = ""; // don't delete this command, it ensures the server works reliably!
  append_HTML_header();
  String IPaddress = WiFi.localIP().toString();
  webpage += "<h3>User Input, if required enter values then select Enter</h3>";
  webpage += "<form action=\"http://" + IPaddress + "\" method=\"POST\">";
  webpage += "Domoticz Server ip<br><input type='text' name='Server' value='"+String(dom_server)+"'><br>";
  webpage += "Domoticz Port Number<br><input type='text' name='Port' value='"+String(dom_port)+"'><br>";
  webpage += "Domoticz Idx<br><input type='text' name='idx' value='"+String(dom_idx)+"'><br>";
  webpage += "Dimmer 1 &nbsp <input type='text' name='IDX1' value='"+String(dim_idx1)+"'> &nbsp""Switch 1 &nbsp <input type='text' name='SW1' value='"+String(sw_idx1)+"'><br>";
  webpage += "Dimmer 2 &nbsp <input type='text' name='IDX2' value='"+String(dim_idx2)+"'> &nbsp""Switch 2 &nbsp <input type='text' name='SW2' value='"+String(sw_idx2)+"'><br>";
  webpage += "Dimmer 3 &nbsp <input type='text' name='IDX3' value='"+String(dim_idx3)+"'> &nbsp""Switch 3 &nbsp <input type='text' name='SW3' value='"+String(sw_idx3)+"'><br>";
  webpage += "Dimmer 4 &nbsp <input type='text' name='IDX4' value='"+String(dim_idx4)+"'> &nbsp""Switch 4 &nbsp <input type='text' name='SW4' value='"+String(sw_idx4)+"'><br>";
  webpage += "Dimmer 5 &nbsp <input type='text' name='IDX5' value='"+String(dim_idx5)+"'> &nbsp""Switch 5 &nbsp <input type='text' name='SW5' value='"+String(sw_idx5)+"'><br>";
  webpage += "Dimmer 6 &nbsp <input type='text' name='IDX6' value='"+String(dim_idx6)+"'> &nbsp""Switch 6 &nbsp <input type='text' name='SW6' value='"+String(sw_idx6)+"'><br>";
  webpage += "Dimmer 7 &nbsp <input type='text' name='IDX7' value='"+String(dim_idx7)+"'> &nbsp""Switch 7 &nbsp <input type='text' name='SW7' value='"+String(sw_idx7)+"'><br>";
  webpage += "Dimmer 8 &nbsp <input type='text' name='IDX8' value='"+String(dim_idx8)+"'> &nbsp""Switch 8 &nbsp <input type='text' name='SW8' value='"+String(sw_idx8)+"'><br>";
  webpage += "Dimmer 9 &nbsp <input type='text' name='IDX9' value='"+String(dim_idx9)+"'> &nbsp""Switch 9 &nbsp <input type='text' name='SW9' value='"+String(sw_idx9)+"'><br>";
  webpage += "Dimmer10 &nbsp <input type='text' name='IDX10' value='"+String(dim_idx10)+"'> &nbsp""Switch10 &nbsp <input type='text' name='SW10' value='"+String(sw_idx10)+"'><br>";
  webpage += "<input type='submit' value='Enter'><br><br>";
  webpage += "</form></body>";
  append_HTML_footer();
  webserver.send(200, "text/html", webpage); // Send a response to the client to enter their inputs, if needed, Enter=defaults
  if (webserver.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < webserver.args(); i++ ) {
      String Argument_Name   = webserver.argName(i);
      String client_response = webserver.arg(i);
      if (Argument_Name == "Server") {
        String Server_response = client_response;
        Serial.println(Server_response);
        Server_response.toCharArray(dom_server, 15);

      }
      if (Argument_Name == "Port") {
        String Port_response = client_response;
        // Serial.println(Port_response);
        Port_response.toCharArray(dom_port, 5);
      }

      if (Argument_Name == "idx") {
        String idx_response = client_response;
        //  Serial.println(idx_response);
        idx_response.toCharArray(dom_idx, 5);

      }

      if (Argument_Name == "IDX1") {
        String IDX1_response = client_response;
        //    Serial.println(IDX1_response);
        IDX1_response.toCharArray(dim_idx1, 5);
      }
      if (Argument_Name == "IDX2") {
        String IDX2_response = client_response;
        //   Serial.println(IDX2_response);
        IDX2_response.toCharArray(dim_idx2, 5);

      }
      if (Argument_Name == "IDX3") {
        String IDX3_response = client_response;
        //   Serial.println(IDX3_response);
        IDX3_response.toCharArray(dim_idx3, 5);
      }
      if (Argument_Name == "IDX4") {
        String IDX4_response = client_response;
        //  Serial.println(IDX4_response);
        IDX4_response.toCharArray(dim_idx4, 5);
      }
      if (Argument_Name == "IDX5") {
        String IDX5_response = client_response;
        //    Serial.println(IDX5_response);
        IDX5_response.toCharArray(dim_idx5, 5);
      }
      if (Argument_Name == "IDX6") {
        String IDX6_response = client_response;
        //  Serial.println(IDX6_response);
        IDX6_response.toCharArray(dim_idx6, 5);
      }
      if (Argument_Name == "IDX7") {
        String IDX7_response = client_response;
        // Serial.println(IDX7_response);
        IDX7_response.toCharArray(dim_idx7, 5);
      }
      if (Argument_Name == "IDX8") {
        String IDX8_response = client_response;
        //  Serial.println(IDX8_response);
        IDX8_response.toCharArray(dim_idx8, 5);
      }
      if (Argument_Name == "IDX9") {
        String IDX9_response = client_response;
        IDX9_response.toCharArray(dim_idx9, 5);
        //  Serial.println(IDX9_response);
      }

      if (Argument_Name == "IDX10") {
        String IDX10_response = client_response;
        IDX10_response.toCharArray(dim_idx10, 5);
        //    Serial.println(IDX10_response);
        Serial.println("Config file writen");
      }

         if (Argument_Name == "SW1") {
        String SW1_response = client_response;
        Serial.println(SW1_response);
        SW1_response.toCharArray(sw_idx1, 5);
      }
      if (Argument_Name == "SW2") {
        String SW2_response = client_response;
        //   Serial.println(IDX2_response);
        SW2_response.toCharArray(sw_idx2, 5);

      }
      if (Argument_Name == "SW3") {
        String SW3_response = client_response;
        //  Serial.println(Switch3_response);
        SW3_response.toCharArray(sw_idx3, 5);
      }
      if (Argument_Name == "SW4") {
        String SW4_response = client_response;
        //  Serial.println(IDX4_response);
        SW4_response.toCharArray(sw_idx4, 5);
      }
      if (Argument_Name == "SW5") {
        String SW5_response = client_response;
        //    Serial.println(IDX5_response);
        SW5_response.toCharArray(sw_idx5, 5);
      }
      if (Argument_Name == "SW6") {
        String SW6_response = client_response;
        //  Serial.println(IDX6_response);
        SW6_response.toCharArray(sw_idx6, 5);
      }
      if (Argument_Name == "SW7") {
        String SW7_response = client_response;
        // Serial.println(IDX7_response);
        SW7_response.toCharArray(sw_idx7, 5);
      }
      if (Argument_Name == "SW8") {
        String SW8_response = client_response;
        //  Serial.println(IDX8_response);
        SW8_response.toCharArray(sw_idx8, 5);
      }
      if (Argument_Name == "SW9") {
        String SW9_response = client_response;
        SW9_response.toCharArray(sw_idx9, 5);
        //  Serial.println(IDX9_response);
      }

      if (Argument_Name == "SW10") {
        String SW10_response = client_response;
        SW10_response.toCharArray(sw_idx10, 5);
        Serial.println("Config file writen");
        SaveConfig();
      }
    }
  }
  webpage = "";
  homepage();
}




void update() {
  String ipaddress = WiFi.localIP().toString();
  webserver.begin();
  myNextion.setComponentText("t2", String(dom_server));
  myNextion.sendCommand("vis p0,1");    // show wifi icon when connected
  myNextion.setComponentText("t10", "Firmware-update-Enabled");
  myNextion.setComponentText("t11", "Pls browse to");
  myNextion.setComponentText("t12", String(ipaddress));
  delay(300);
}
void resetwifi() {
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.reset();

}

void batlvl() {    // Compute currect battery voltage and update the display  ///
  raw = analogRead(A0);
  volt = raw / 1023.0;
  volt = volt * 4.3;
  sensorValue = analogRead(analogInPin);
  // map it to the range of the analog out:
  output_value = map(sensorValue, 800, 1023, 0, 100);
  myNextion.setComponentValue("j2", output_value);
  myNextion.setComponentValue("j1", output_value);
  // myNextion.setComponentValue("n10", volt);
  myNextion.setComponentText("t10", String(volt));
  //  Serial.print(volt);
  //  Serial.print(".");
  Serial.print(output_value);
}

void printInfo() {
  // Domoticz format /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=udevice&idx=");
  client.print(dom_idx);
  client.print("&nvalue=0&svalue=");
  client.print(volt);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim1() {
  
  int dimmer1 = myNextion.getComponentValue("dimmer.n0");
  Serial.println("Dimmer 1 val:");
  Serial.print(dimmer1);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx1);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer1);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim2() {

  int dimmer2 = myNextion.getComponentValue("dimmer.n1");
  Serial.println("Dimmer 2 val:");
  Serial.print(dimmer2);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx2);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer2);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim3() {
  //////////////////////DIMER 3//////////////////////////
  int dimmer3 = myNextion.getComponentValue("dimmer.n2");
  Serial.println("Dimmer 3 val:");
  Serial.print(dimmer3);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx3);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer3);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim4() {
  //////////////////////Dimmer 4/////////////////////////
  int dimmer4 = myNextion.getComponentValue("dimmer.n3");
  Serial.println("Dimmer 4 val:");
  Serial.print(dimmer4);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx4);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer4);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim5() {
  //////////////////////DIMMER 5///////////////////////////
  int dimmer5 = myNextion.getComponentValue("dimmer2.n4");
  //Serial.println("Keuken :");
  Serial.print(dimmer5);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx5);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer5);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim6() {
  //////////////////////Dimmer 6///////////////////////////
  int dimmer6 = myNextion.getComponentValue("dimmer2.n5");
  Serial.println("Dimmer 6  :");
  Serial.print(dimmer6);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx6);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer6);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim7() {
  //////////////////////Dimmer 7///////////////////////////
  int dimmer7 = myNextion.getComponentValue("dimmer2.n6");
  Serial.println("Dimmer 7  :");
  Serial.print(dimmer7);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
    client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx7);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer7);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim8() {
  //////////////////////Dimmer 8///////////////////////////
  int dimmer8 = myNextion.getComponentValue("dimmer2.n7");
  Serial.println("Dimmer 8  :");
  Serial.print(dimmer8);
  Serial.println("");
  WiFiClient client;
  if (client.connect(dom_server, httpport))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(dim_idx8);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer8);
  client.print(";");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(dom_server);
  client.print(":");
  client.println(dom_port);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}



void loop() {
  // put your main code here, to run repeatedly:
  webserver.handleClient();
  ftp.handleFTP();        //make sure in loop you call handleFTP()!!
  //if(volt != last_volt){  // if nothing has changed, do nothing
  //   last_volt = volt;
  //   batlvl();
  //}

  //Time loop
  unsigned long currentMillis = millis(); // grab current time
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
    Serial.print("5 min update ");
    batlvl();
    printInfo();
    previousMillis = millis();
  }


  /////////////////////////////Switch 1/////////////////////////////////

  String message = myNextion.listen(); //check for message
  if (message != "") { // if a message is received...
    Serial.println(message); //...print it out
  }
  if (message == "sw1on") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx1);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);
    // einde

  }

  if (message == "sw1off") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    tmpidx = atoi(sw_idx1);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }
  /////////////////////////Fw Update enable//////////////////////////////////////

  if (message == "update") {
    delay(50);
    ++value;
    update();

  }
  /////////////////////////////Switch 2/////////////////////////////////

  if (message == "sw2on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    tmpidx = atoi(sw_idx2);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);
  }

  if (message == "sw2off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    tmpidx = atoi(sw_idx2);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 3/////////////////////////////////

  if (message == "sw3on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    //   String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=On";  /////////////////////////////////////////////////////////////////////////////////
    tmpidx = atoi(sw_idx3);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";
    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw3off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    tmpidx = atoi(sw_idx3);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }

  /////////////////////////////Switch 4/////////////////////////////////

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "sw4on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx4);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }

  if (message == "sw4off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx4);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  /////////////////////////////Switch 5/////////////////////////////////

  if (message == "sw5on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    tmpidx = atoi(sw_idx5);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw5off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    tmpidx = atoi(sw_idx5);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 6/////////////////////////////////

  if (message == "sw6on") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }
    tmpidx = atoi(sw_idx6);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw6off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx6);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 7/////////////////////////////////

  if (message == "sw7on") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx7);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw7off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx7);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  /////////////////////////////Switch 8/////////////////////////////////

  if (message == "sw8on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx8);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }


  if (message == "sw8off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx8);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 9/////////////////////////////////
  if (message == "sw9on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx9);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw9off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx9);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";
    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 10/////////////////////////////////

  if (message == "sw10on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }


    tmpidx = atoi(sw_idx10);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw10off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    tmpidx = atoi(sw_idx10);
    String url = "/json.htm?type=command&param=switchlight&idx=" + tmpidx + "&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////Switch 11/////////////////////////////////

  if (message == "sw11on") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "sw11off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(dom_server, httpport)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dom_server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  if (message == "Dimmer1") {
    Serial.print("Dimmer1");
    Dim1();
  }

  if (message == "Dimmer2") {
    Serial.print("Dimmer2");
    Dim2();

  }

  if (message == "Dimmer3") {
    Serial.print("Dimmer3");
    Dim3();
  }

  if (message == "Dimmer4") {
    Serial.print("Dimmer4");
    Dim4();
  }

  if (message == "Dimmer5") {
    Serial.print("Dimmer5");
    Dim5();
  }

  if (message == "Dimmer6") {
    Serial.print("Dimmer6");
    Dim6();
  }

  if (message == "Dimmer7") {
    Serial.print("Dimmer7");
    Dim7();

  }

  if (message == "Dimmer8") {
    Serial.print("Dimmer8");
    Dim8();
  }

    if (message == "update1") {
    // delay(50);
    batlvl();
    printInfo();
    ++value;

  }
  if (message == "reset") {
    Serial.print("Reset wifi");
    //SaveConfig();
     resetwifi();

  }
}

