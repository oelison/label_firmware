#ifndef WebPage_h
#define WebPage_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Update.h>
#include "mbedtls/base64.h"
#include <vector>
#include "NVMData.h"
#include "DynamicData.h"

class WebPage
{
private:
    WebServer server;
    String GenHeader(int redirectTime);
    String GenFooter();
    String GenTableStart();
    String GenTableNewColumn();
    String GenTableRows(String Content[], int Count);
    String GenTableEnd();
    String addKeyValuePair(String key, String value);
    void handleRoot();
    void handleUploadJson();
    void handleChange();
    void handlePrint();
    void handleNotFound();
    void handleFirmware();
    void handleUpload();
    void handleUpload2();
    void handleJson();
    String setMessage(String arg, String value, String result);
public:
    WebPage();
    ~WebPage();
    void Init();
    void loop();
    bool newNetworkSet = false;
    bool startPrinting = false;
    u32_t lostSteps = 0;
    uint8_t printBuffer[12 * 3000]; // 2700 Zeilen a 12 Byte (150 in front and 150 at end for margin)
    u32_t bytesReceived = 0;
    u32_t printBufferPointer = 0;
    uint32_t length = 0;
};
WebPage::WebPage()
{
}
WebPage::~WebPage()
{
}
WebServer server(80);
void WebPage::loop() {
    server.handleClient();
}
void WebPage::Init() {
    
    server.on("/", std::bind(&WebPage::handleRoot, this));
    server.on("/change", std::bind(&WebPage::handleChange, this));
    server.on("/print", std::bind(&WebPage::handlePrint, this));
    server.on("/uploadjson", HTTP_POST, std::bind(&WebPage::handleUploadJson, this));
    // choose bin file
    server.on("/firmware", HTTP_GET, std::bind(&WebPage::handleFirmware, this));
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, std::bind(&WebPage::handleUpload, this), std::bind(&WebPage::handleUpload2, this));
    server.onNotFound(std::bind(&WebPage::handleNotFound, this));
    server.begin();
}
String WebPage::GenHeader(int redirectTime)
{
  String message= "";
  message += "<html>";
  message += "<head>";
  if (redirectTime > 0)
  {
    String redirectTimeString = String(redirectTime);
    message += "<meta http-equiv=\"refresh\" content=\"" + redirectTimeString + ";url=http://" + DynamicData::get().ipaddress + "/\" />";
  }
  message += "</head>";
  message += "<body>\n";
  message += "\t<p>Casio KL-780</p>\n";
  message += "\t<p>---------------------------</p>\n";
  return message;
}
String WebPage::GenFooter()
{
  String message= "";
  message += "\t<p>---------------------------</p>\n";
  message += "</body>";
  message += "</html>\n";
  return message;
}
String WebPage::GenTableStart()
{
  String message= "";
  message += "\t<table border=\"4\">";
  message += "\t<tr>\n";
  return message;
}
String WebPage::GenTableNewColumn()
{
  String message= "";
  message += "\t</tr>";
  message += "\t<tr>\n";
  return message;
}
String WebPage::GenTableRows(String Content[], int Count)
{
  String message = "";
  for (int i = 0; i < Count; i++)
  {
    message += "<td>"+Content[i]+"</td>";
  }
  message +="\n";
  return message;
}
String WebPage::GenTableEnd()
{
  String message= "";
  message += "\t</tr>";
  message += "\t</table>\n";
  return message;
}
void WebPage::handleNotFound() {
  String message= "";
  message += GenHeader(3);
  message += "File Not Found\n\n";
  message += GenFooter();
  server.send(200, "text/html", message);
}
void WebPage::handleChange() {
  String message = "Ohh oh!\n\n";
  bool netNameSet = false;
  bool netPasswordSet = false;
  String netName = "";
  String netPassword = "";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "netname")
    {
      netNameSet = true;
      netName = server.arg(i);
    }
    else if (server.argName(i) == "password")
    {
      netPasswordSet = true;
      netPassword = server.arg(i);
    }
  }
  if ((netPasswordSet == true)&&(netNameSet == true))
  {
    newNetworkSet = true;
    NVMData::get().SetNetData(netName, netPassword);
    message = "You got it!";
  }
  String returnMessage= "";
  returnMessage += GenHeader(3);
  returnMessage += message;
  returnMessage += "</body>";
  returnMessage += "</html>";
  server.send(200, "text/html", returnMessage);
}

// Decode Base64 String zu Byte-Array
std::vector<uint8_t> base64Decode(const String& input) {
    size_t out_len = 0;
    // Erst die Länge ermitteln
    mbedtls_base64_decode(nullptr, 0, &out_len, (const uint8_t*)input.c_str(), input.length());

    std::vector<uint8_t> buffer(out_len);
    mbedtls_base64_decode(buffer.data(), buffer.size(), &out_len, (const uint8_t*)input.c_str(), input.length());

    return buffer;
}

void WebPage::handleUploadJson() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", "Bad JSON");
    return;
  }

  size_t index = doc["index"];
  const char *b64 = doc["data"];

  std::vector<uint8_t> decoded = base64Decode(b64);
  size_t outLen = decoded.size();  // Anzahl der Bytes
  if (index + outLen > sizeof(printBuffer)) {
    server.send(413, "text/plain", "Payload Too Large");
    return;
  }
  // Dann in dein printBuffer kopieren:
  memcpy(printBuffer + index, decoded.data(), outLen);
  bytesReceived += outLen;
  printBufferPointer = index + outLen;
  Serial.printf("UploadJson: Block empfangen @%d (%d Bytes)\n", index, outLen);
  server.send(200, "text/plain", "OK");
}

void WebPage::handlePrint() {
    String message = "Ohh oh!\n\n";
    startPrinting = false;
    for (uint8_t i = 0; i < server.args(); i++)
    {
        if (server.argName(i) == "length")
        {
            startPrinting = true;
            message = "Start print!\n\n";
            length = server.arg(i).toInt();
            Serial.printf("Print length: %d\n", length);
            Serial.println("Start print");
        }
    }
    String returnMessage= "";
    returnMessage += GenHeader(1);
    returnMessage += message;
    returnMessage += "</body>";
    returnMessage += "</html>";
    server.send(200, "text/html", returnMessage);
}
void WebPage::handleFirmware() {
  const char* serverIndex = 
    "<script src='https://sciphy.de/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
      "<input type='file' name='update'>"
            "<input type='submit' value='Update'>"
        "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
      "$('form').submit(function(e){"
      "e.preventDefault();"
      "var form = $('#upload_form')[0];"
      "var data = new FormData(form);"
      " $.ajax({"
      "url: '/update',"
      "type: 'POST',"
      "data: data,"
      "contentType: false,"
      "processData:false,"
      "xhr: function() {"
      "var xhr = new window.XMLHttpRequest();"
      "xhr.upload.addEventListener('progress', function(evt) {"
      "if (evt.lengthComputable) {"
      "var per = evt.loaded / evt.total;"
      "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
      "}"
      "}, false);"
      "return xhr;"
      "},"
      "success:function(d, s) {"
      "console.log('success!')" 
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", serverIndex);
}
void WebPage::handleUpload() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}
void WebPage::handleUpload2() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    //Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    /* flashing firmware to ESP*/
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      //Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}
void WebPage::handleRoot() {
    String uri = DynamicData::get().ipaddress;
    String message = "";
    message += GenHeader(0);
    
    if( DynamicData::get().setNewNetwork == true )
    {
        message += "\t<form action=\"change\">\n";
        message += "\t<label class=\"h2\" form=\"networkdata\">Network name</label>\n";
        message += "\t<div>\n";
        message += "\t<label for=\"netname\">netname</label>\n";
        message += "\t<input type=\"text\" name=\"netname\" maxlength=\"30\">\n";
        message += "\t</div>\n";
        message += "\t<div>\n";
        message += "\t<label for=\"password\">password</label>\n";
        message += "\t<input type=\"text\" name=\"password\" maxlength=\"40\">\n";
        message += "\t</div>\n";
        message += "\t<div>\n";
        message += "\t<button type=\"reset\">clear</button>\n";
        message += "\t<button type=\"submit\">set</button>\n";
        message += "\t</div>\n";
        message += "\t</form>\n";
    }
    message += "\t<p>---------------------------</p>\n";
    message += "<button onclick=\"testUploadJson()\">Upload JSON testen</button>\n";
    message += "<script>";
    message += "async function testUploadJson() {";
    message += "  const bytes = new Uint8Array([1,2,3,4,5,6,7,8,9,10]);";  // Dummy-Daten
    message += "  const b64 = btoa(String.fromCharCode(...bytes));";
    message += "  const body = JSON.stringify({ index: 0, data: b64 });";
    message += "  const response = await fetch('/uploadjson', {";
    message += "    method: 'POST',";
    message += "    headers: { 'Content-Type': 'application/json' },";
    message += "    body";
    message += "  });";
    message += "  if (response.ok) { alert('UploadJson Test erfolgreich!'); }";
    message += "  else { alert('Fehler beim UploadJson!'); }";
    message += "}";
    message += "</script>";
    message += "\t<p>---------------------------</p>\n";
    message += "\t<form action=\"changeoffset\">\n";
    message += "\t<label class=\"h2\" form=\"curoff\">current offset [mA]</label>\n";
    message += "\t<div>\n";
    message += "\t<button type=\"reset\">clear</button>\n";
    message += "\t<button type=\"submit\">set</button>\n";
    message += "\t</div>\n";
    message += "\t</form>\n";
    message += "\t<p>---------------------------</p>\n";
    message += "\t<p>lostSteps: " + String(lostSteps) + "</p>\n";
    message += "\t<p>received data: " + String(bytesReceived) + "</p>\n";
    message += "\t<p>printBufferPointer: " + String(printBufferPointer) + "</p>\n";
    message += "\t<p>ipaddress:  " + DynamicData::get().ipaddress + "</p>\n";
    message += "\t<p>---------------------------</p>\n";
    message += GenFooter();
    server.send(200, "text/html", message);
}

String WebPage::addKeyValuePair(String key, String value)
{
  String retVal = "";
  retVal += "\"";
  retVal += key;
  retVal += "\" :\"";
  retVal += value;
  retVal += "\"";
  return retVal;
}
void WebPage::handleJson()
{
  String message = "";
  String key = "";
  message += "{ ";
  message += addKeyValuePair("Voltage", String(DynamicData::get().busVoltage, 3));
  message += ",";
  message += addKeyValuePair("Current", String(DynamicData::get().current, 3));
  message += ",";
  message += addKeyValuePair("Power", String(DynamicData::get().power, 3));
  message += "}";
  server.send(200, "text/plain", message);
}
#endif