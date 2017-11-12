#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include "WiFiManager.h"
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

unsigned long interval = 300000; // 300000 = 5 min interval
unsigned long previousMillis = 0; // millis() returns an unsigned long.

unsigned int raw = 0;
float volt = 0.0;

String v = String(volt); // change float into string
//int old_sensor_value = 0;
const int analogInPin = A0;

int sensorValue = 0;        // value read from analog input
int output_value = 0;        // value output to display


////Dimmer IDX numbers ////////

int idx = 377; //IDX for virtual sensor, found in Setup -> Devices
int idx1 = 363; // IDX FOR Vensterbank
int idx2 = 358; // ikea lamp
int idx3 = 376;  //tafellamp
int idx4 = 255;  //aqua rw
int idx5 = 256;  // aqua bw
int idx6 = 220; //keuken warm
int idx7 = 376; //tafel lamp
int idx8 = 107; //blokhut

const char* host1 = "192.168.1.3";   //Domoticz server ip 
const int httpPort = 8084;  // Domoticz port
const char* host = "Domoremote";  // just the host name that shows up on the network
int value = 0;
WiFiManager wifi;

String ssid;  //string that holds ssid
String pass;  // string that holds the password
String rssi;  // string that holds the signal strenght
String ip2;   //Client ip addres
String apssid;  // Acces point ip addres

ESP8266WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";


SoftwareSerial nextion(5, 4);   // set rx and tx to  pins 5 and 4

Nextion myNextion(nextion, 9600);//create a Nextion object named myNextion using the nextion serial port @ 9600bps
//myNextion.init();

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  ip2 = WiFi.softAPIP().toString();
  apssid = wifi.getConfigPortalSSID();
  Serial.println(WiFi.softAPIP());
  myNextion.sendCommand("page wifi");
  myNextion.setComponentText("t15", "Please connect to network:");
  myNextion.setComponentText("t18", "And browse to :");
  myNextion.setComponentText("t17",ip2);
  myNextion.setComponentText("t16",apssid);

  //if you used auto generated SSID, print it
  Serial.println(apssid);
 
}



//Nextion myNextion(nextion, 9600);//create a Nextion object named myNextion using the nextion serial port @ 9600bps

void setup() {

  myNextion.init();
  pinMode(A0, INPUT);
  int sensorValue = 0;
  Serial.begin(9600);
  WiFiManager wifiManager;
  ssid = WiFi.SSID();
  const int analogRead(A0);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("Remote-Ap")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(10);

  }

  while (WiFi.status() != WL_CONNECTED) {

   // delay(100);
    Serial.print(".");
    Serial.print("connecting to ");
    Serial.println(host1);
  }
  
    String ipaddress = WiFi.localIP().toString();
  // String rssi = WiFi.RSSI().toString();
  // Serial.println(WiFi.RSSI());
  myNextion.sendCommand("page home");
  myNextion.setComponentText("t1", String(ssid));
  myNextion.setComponentText("t4", String(ipaddress));
  myNextion.sendCommand("t10.val,10" );
 myNextion.setComponentValue("j2", output_value);
  myNextion.sendCommand("va0.val,1");
    myNextion.sendCommand("vis p0,1");

  
  MDNS.begin(host);
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", serverIndex);
  });
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.reset();
  }, []() {
    HTTPUpload& upload = server.upload();

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
void update() {
  String ipaddress = WiFi.localIP().toString();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  myNextion.setComponentText("t2", String(host1));
  myNextion.sendCommand("vis p0,1");
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

void batlvl() {
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
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=udevice&idx=");
  client.print(idx);
  client.print("&nvalue=0&svalue=");
  client.print(volt);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim1() {
  // Domoticz format /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT
  //  /json.htm?type=command&param=switchscene&idx=3&switchcmd=Off
  //Set%20Level&level=6
  int dimmer1=myNextion.getComponentValue("dimmer.n0");
  Serial.println("Dimmer 1 val:");
  Serial.print(dimmer1);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx1);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer1);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim2() {
  // Domoticz format /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT
  //  /json.htm?type=command&param=switchscene&idx=3&switchcmd=Off
  //Set%20Level&level=6
  int dimmer2=myNextion.getComponentValue("dimmer.n1");
  Serial.println("Dimmer 2 val:");
  Serial.print(dimmer2);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx2);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer2);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim3() {
 //////////////////////Aquarium Dimmer red/ white///////////////////////////   
  int dimmer3=myNextion.getComponentValue("dimmer2.n4");
  Serial.println("Aqua RW val:l:");
  Serial.print(dimmer3);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx4);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer3);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim4() {
 //////////////////////Aquarium Dimmer blue/ white///////////////////////////   
  int dimmer4=myNextion.getComponentValue("dimmer2.n5");
  Serial.println("Aqua BW val:");
  Serial.print(dimmer4);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx5);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer4);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim5() {
 //////////////////////keuken///////////////////////////   
  int dimmer5=myNextion.getComponentValue("dimmer.n3");
  Serial.println("Keuken :");
  Serial.print(dimmer5);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx6);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer5);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}

void Dim6() {
 //////////////////////keuken///////////////////////////   
  int dimmer6=myNextion.getComponentValue("dimmer.n2");
  Serial.println("Tafel lamp :");
  Serial.print(dimmer6);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx7);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer6);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}
void Dim7() {
 //////////////////////blokhut///////////////////////////   
  int dimmer7=myNextion.getComponentValue("dimmer2.n7");
  Serial.println("Blokhut :");
  Serial.print(dimmer7);
  Serial.println("");
  WiFiClient client;
  if (client.connect(host1, httpPort))
  client.print("GET /json.htm?type=command&param=switchlight&idx=");
  client.print(idx8);
  client.print("&switchcmd=Set%20Level&level=");
  client.print(dimmer7);
  client.print(";");
  //client.print(humidity);
  // client.print(";0"); //Value for HUM_STAT. Can be one of: 0=Normal, 1=Comfortable, 2=Dry, 3=Wet
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(host1);
  client.print(":");
  client.println(httpPort);
  client.println("User-Agent: Arduino-ethernet");
  client.println("Connection: close");
  client.println();
  client.stop();
}





////////////////////////////////////Main Loop//////////////////////////////////////////////////////////
void loop(void) {

  server.handleClient();
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


 
  ///////////////////////////////////Tafel on//////////////////////////////////////////////////////

  String message = myNextion.listen(); //check for message
  if (message != "") { // if a message is received...
    Serial.println(message); //...print it out
  }
  if (message == "tafelon") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchlight&idx=376&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);
    // einde

  }

  if (message == "tafeloff") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    String url = "/json.htm?type=command&param=switchlight&idx=376&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }
  /////////////////////////Fw Update enable////////////////////////////////////////////////////////////////////////
  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "update") {
    delay(50);
    ++value;
    update();

    // myNextion.setComponentText("t2",String(host));

    // Use WiFiClient class to create TCP connections
    // WiFiClient client;

    // if (!client.connect(host, httpPort)) {
    //   Serial.println("connection failed");
    // return;
    // }

    // We now create a URI for the request
    // domoticz or url you want to hit
    // String url = "/json.htm?type=command&param=switchlight&idx=90&switchcmd=Off";

    //  Serial.print("Requesting URL: ");
    //  Serial.println(url);

    // This will send the request to the server
    //client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    //           "Host: " + host + "\r\n" +
    //         "Connection: close\r\n\r\n");
    // delay(10);
  }
  ////////////// Tv lamp ////////////////////////////////////////////////////////

  if (message == "tvon") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchlight&idx=47&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);
  }

  if (message == "tvoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    String url = "/json.htm?type=command&param=switchlight&idx=47&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  ////////////////////////////////Aqua Button /////////////////////////////////////////////////////////////

  if (message == "aquaon") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "aquaoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }
  /////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////Keuken Warm///////////////////////////////////////////////////////////////////

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "keukenon") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=220&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "keukenoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=220&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  ///////////////////////////////////////Vensterbank/////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "vensteron") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchlight&idx=363&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "vensteroff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=363&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  ///////////////////////////////////Blokhut  107 //////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "blokhuton") {
    delay(50);
    ++value;

    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    String url = "/json.htm?type=command&param=switchlight&idx=107&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "blokhutoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=107&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  //////////////////////////////////////Kamer off 107 //////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "blokhut12off") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=107&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  ///////////////////////////////////////////////Trap  247/////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "trapon") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=247&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "trapoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=247&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  //////////////////////////////////Kamer lamp //////////////////////////////////////////////////////////////////////////
  if (message == "kameron") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=358&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "kameroff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchlight&idx=358&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////////  Kamer scene  //////////////////////////

  if (message == "kamerson") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchscene&idx=1&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "kamersoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchscene&idx=1&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }
  /////////////////////////////////  Aqua scene  //////////////////////////

  if (message == "aquason") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=On";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(0);

  }

  // Looks if received msg is button
  //  65 [psge id] [buttun nr} ff ff ff
  if (message == "aquasoff") {
    delay(50);
    ++value;

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host1, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    // domoticz or url you want to hit
    String url = "/json.htm?type=command&param=switchscene&idx=3&switchcmd=Off";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host1 + "\r\n" +
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

 if (message == "Dimmer5") {
     Serial.print("Aqua r/w");
     Dim3();
 }

 if (message == "Dimmer6") {
     Serial.print("Aqua b/w");
     Dim4();
      }

 if (message == "Dimmer4") {
     Serial.print("Keuken");
     Dim5();
 }

  if (message == "Dimmer3") {
     Serial.print("Tafel lamp");
     Dim6();
 }

  if (message == "Dimmer7") {
     Serial.print("Blokhut");
     Dim7();
     
 }
   if (message == "update1") {
    // delay(50);
    batlvl();
    printInfo();
    ++value;

  }
if (message == "reset") {
     Serial.print("Reset wifi");
     resetwifi();
     
 
}
}

//end


