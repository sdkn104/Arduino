/*
  MyCockpit

  This library is released under LGPL.
  This library uses codes from FSWebServer which is released uner LGPL.
  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/examples/FSBrowser

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <MyCockpit.h>

#define DBG_OUTPUT_PORT DebugOut

#define SERVER_PORT_NO 80  // should be 80 to be looked by Web Browser

ESP8266WebServer server(SERVER_PORT_NO);

//holds the current upload
File fsUploadFile ;

FTPClient Ftp;

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

// ------------ Handler functions for http requests -----------------------------

// read file and send the contents to client
bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    //DBG_OUTPUT_PORT.println("opening file {" + path * "}");
    File file = SPIFFS.open(path, "r");
    if( !file ) {
      DBG_OUTPUT_PORT.println("ERROR: fail to open file {" + path + "}"); 
      return false; 
    }
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  } else {
    DBG_OUTPUT_PORT.println("ERROR: file not exists {" + path + "}");
    return false;
  }
}

// get file from client (by POST) and save as a file
void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print(" Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    if(!fsUploadFile) {
      DBG_OUTPUT_PORT.println("Error: cannot open file to write");
    }
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {
      fsUploadFile.close();
      server.send(200, "text/plain", "uploaded");
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}


void handleFileCopy(){
  if(server.args() < 2) return server.send(500, "text/plain", "BAD ARGS");
  String path1 = server.arg(0);
  String path2 = server.arg(1);
  DBG_OUTPUT_PORT.println("handleFileCopy: " + path1 + " " + path2);
  if(path1 == "/" || path2 == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path1))
    return server.send(404, "text/plain", "FileNotFound");
  if(SPIFFS.exists(path2))
    return server.send(404, "text/plain", "File Exists");
  File file1 = SPIFFS.open(path1, "r");
  if(!file1) return server.send(500, "text/plain", "OPEN FAILED");
  File file2 = SPIFFS.open(path2, "w");
  if(!file2) {
    file1.close();
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  while (file1.available()) {
    int t = file1.read();
    if( t == -1 ) break; 
    file2.write(t);
  }
  file1.close();
  file2.close();
  server.send(200, "text/plain", "OK");
  path1 = String();
  path2 = String();
}


void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  if( !SPIFFS.remove(path))
    return server.send(500, "text/plain", "FAIL TO REMOVE");
  server.send(200, "text/plain", "OK");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "OK");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output2 = "<table>";
  while(dir.next()){
    File entry = dir.openFile("r");
    bool isDir = false;
    output2 += String("<tr><td><a href=\"") +  entry.name() + "\">" + entry.name() + "</a></td><td style=\"padding:0px 20px;text-align:right\">" + entry.size() + "</td></tr>";
    entry.close();
  }
  output2 += "</table>";
  server.send(200, "text/html", output2);
}

void handleCommand(){
  if(server.args() < 1) return server.send(500, "text/plain", "BAD ARGS");
  String cmd = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCommand: " + cmd);
  if(cmd == "")
    return server.send(500, "text/plain", "No command spplied");
  String out = String("");
  if(String(cmd)=="help" ) {
      out = "available commands:\nhelp\ndate\nstatus\n";
  } else if(String(cmd) == "date") {
      out = String("ESP Date(NTP): ") + getDateTimeNow();
  } else if(String(cmd)=="status" ) {
      out = String("status:\n") + getStatus();
  } else {
      out = String("command not found: ") + cmd;
  }
  server.send(200, "text/plain", out);
  cmd = String();
}


// show file uploader
void handleFileUploader() {  
  DBG_OUTPUT_PORT.println("handleFileUploader");
  String output = "";
  output += "<!DOCTYPE html>";
  output += "<html><head><meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\">";
  output += "<title>ESP8266 Uploader</title></head><body>";
  output += "<h2>ESP8266 File Uploader</h2>";
  output += "<form method='POST' action='/edit' enctype='multipart/form-data'>";
  output += "<input type='submit' value='Upload'>";
  output += "<input type='file' name='upload'>";
  output += "</form>";
  output += "<form action='/ftpGetInitFiles'>";
  output += "<input type='submit' value='FTP Get Init Files'>";
  output += "<input type='text' name='files' value='index.htm|ftp.htm|'>";
  output += "</form></html>";
  server.send(200, "text/html", output);
  output = String();
}

// ---------- Customization of the Costom Web Page -------------------------------------------------

// head part of HTML of the custom page
String customHtml_head = String("<!DOCTYPE html>")
//  + "<html><head><meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\">"
//  + "<title>ESP8266 Custom Page</title></head><body>"
//  + "<h2>ESP8266 Custom</h2>";
  + "<html><head>"
  + "<meta charset=\"utf-8\">"
  + "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">"
  + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  + "<title>ESP8266 Cockpit</title>"
  + "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\">"
  + "</head><body style=\"margin:5px\">"
  + "<ul class=\"nav nav-pills\"><li role=\"presentation\"><a href=\"index.htm\">Home</a></li><li role=\"presentation\"><a href=\"/ftp.htm\">FTP</a></li><li role=\"presentation\" class=\"active\"><a href=\"/custom.htm\">Custom</a></li></ul>"
  + "<h2>ESP8266 - Costom Page</h2>";

// middle part of HTML of the custom page (to be filled by addMyCockpit(), etc.)
String customHtml_mid = String();

// tail part of HTML of the custom page
String customHtml_tail = String("</html>");


// add custom item ( uri and callback )
//   should be called before setupMyCockpit()
void addMyCockpit(const char* uri, ESP8266WebServer::THandlerFunction handler){
  // add server on
  server.on(uri, HTTP_GET, handler);
  // add to custom home page
  customHtml_mid += String()
             + "<form action='" + uri + "'>"
             + "<input type='submit' value='" + String(uri).substring(1) + "'>"
             + "</form>";
}

void addMyCockpit(const char* uri, int argnum, ESP8266WebServer::THandlerFunction handler){
  // add server on
  server.on(uri, HTTP_GET, handler);
  // add to custom home page
  customHtml_mid += String() + "<br><form action='" + uri + "' class=\"form-inline\"><div class=\"form-group\"><div class=\"input-group\">";
  for(int i=0; i<argnum; i++){
    customHtml_mid += String() + "<input type='text' class=\"form-control\" name='arg" + i + "' style=\"width:20ex\">";
  }
  customHtml_mid += String() + "<span class=\"input-group-btn\"><button type='submit' class=\"btn btn-default\">" + String(uri).substring(1) + "</button></span>"
             + "</div></div></form>\r\n\r\n";
}


// add specified HTML to custom page
void addHtmlMyCockpit(String html){
  customHtml_mid += html;
}

// setup 
void setupMyCockpit(void){
  if( !SPIFFS.begin() ) DBG_OUTPUT_PORT.println("Error: fail to mount SPIFFS");

  //-------- CREATE CUSTOM PAGE -------------------------------------------------------
  String customHtml = customHtml_head +
             + "<p class=\"bg-info\" style=\"padding:1ex\">SKETCH: "+getThisSketch()+"</p>\r\n"
             + customHtml_mid;

  // write to custom.htm
  File file = SPIFFS.open("/custom.htm", "w");
  if(file) {
    file.println(customHtml);
    file.println(customHtml_tail);
    file.close();
  } else {
    DBG_OUTPUT_PORT.println("OPEN FAILED");
  }
  customHtml = "";

  //-------- SERVER INIT -------------------------------------------------------

  // ----- File System -----
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  server.on("/json/ls", HTTP_GET, [](){
    server.send(200, "text/json", jsonFileList("/"));
  });

  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "editor edit.htm Not Found");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/create", HTTP_GET, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  server.on("/delete", HTTP_GET, handleFileDelete);
  //upload file
  //  first callback is called after the request has ended with all parsed arguments
  //  second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", "uploading... "); }, handleFileUpload);
  // file copy
  server.on("/copy", HTTP_GET, handleFileCopy);
  // print file system info
  server.on("/du", HTTP_GET, [](){
    server.send(404, "text/plain", getFSInfo());
  });
  // refresh
  server.on("/refresh", HTTP_GET, [](){
    String log = refreshFS("refr");
    server.send(404, "text/plain", log);
  });
  server.on("/ftpGetInitFiles", HTTP_GET, [](){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String files = server.arg(0);
    String log = ftpGetInitFiles("", files);
    server.send(404, "text/plain", log);
  });
  // format FS
  server.on("/format", HTTP_GET, [](){
    DebugOut.println("Formatted SPIFFS");
    SPIFFS.format();
    //server.send(200, "text/plain", "formatted SPIFFS.");
    server.send(200, "text/html", "<html><body>formatted SPIFFS<br><br><a href='/restart'>Restart ESP</a></body></html>");
  });

  // ----- System -----
  // print system info
  server.on("/p", HTTP_GET, [](){
    server.send(200, "text/plain", getSystemInfo());
  });
  // software restart
  server.on("/restart", HTTP_GET, [](){
    DebugOut.println("Restart ESP...");
    server.send(200, "text/html", "<html><body>restarting ESP...<br><br><a href='/'>Home</a></body></html>");
    server.close();
    jsonConfig.save();
    SPIFFS.end();
    ESP.restart();
  });
  server.on("/shutdown", HTTP_GET, [](){
    jsonConfig.save();
    SPIFFS.end();
    server.send(200, "text/plain", "Ready to power off. System will restart in 30 sec.");
    server.close();
    delay(30000);
    ESP.restart();
  });

  // ----- Appication -----
  // execute command
  server.on("/cmd", HTTP_GET, handleCommand);
  // DebugOut
  server.on("/debugoutserial", HTTP_GET, [](){
    DebugOut.setToSerial();
    if( jsonConfig.available() ) {
      jsonConfig.obj()["DebugOut"] = DebugOut.getType();
      jsonConfig.save();
    }
    server.send(200, "text/plain", "OK");
  });
  server.on("/debugoutnull", HTTP_GET, [](){
    DebugOut.setToNull();
    if( jsonConfig.available() ) {
      jsonConfig.obj()["DebugOut"] = DebugOut.getType();
      jsonConfig.save();
    }
    server.send(404, "text/plain", "OK");
  });
  server.on("/debugoutfile", HTTP_GET, [](){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    if( path == "" ) DebugOut.setToFile();
    else             DebugOut.setToFile(path);
    if( jsonConfig.available() ) {
      jsonConfig.obj()["DebugOut"] = DebugOut.getType();
      jsonConfig.save();
    }
    server.send(404, "text/plain", "OK");
  });
  server.on("/wolan", HTTP_GET, [](){
    sendWoLtoToshiyukiPC();
    server.send(200, "text/plain", "OK");
  });
  server.on("/showConfig", HTTP_GET, [](){
    String s;
    if( jsonConfig.available() ) {
      jsonConfig.obj().prettyPrintTo(s);
      jsonConfig.save();
    } else {
      s = "Json config not loaded";
    }
    server.send(200, "text/plain", s + ", OK");
  });
  server.on("/setConfig", HTTP_GET, [](){
    if(server.args() != 2) return server.send(500, "text/plain", "BAD ARGS");
    String key = server.arg(0);
    String val = server.arg(1);
    String val0 = String("0")+val;
    long   num = val0.toInt();
    double dbl = val0.toFloat();
    if( jsonConfig.available() ) {
      if( String(num) == val ) {
        jsonConfig.obj()[key] = num;
      } else if( dbl == (val0+"0").toFloat() && dbl != (val0+"1").toFloat() ) {
        jsonConfig.obj()[key] = dbl;
      } else {
        jsonConfig.obj()[key] = val;
      }
      jsonConfig.save();
      jsonConfig.flush();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "json not loaded yet");
    }
  });
  server.on("/deleteConfig", HTTP_GET, [](){
    if(server.args() != 1) return server.send(500, "text/plain", "BAD ARGS");
    String key = server.arg(0);
    if( jsonConfig.available() ) {
      jsonConfig.obj().remove(key);
      jsonConfig.save();
      jsonConfig.flush();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "json not loaded yet");
    }
  });
  server.on("/json/showConfig", HTTP_GET, [](){
    if( jsonConfig.available() ) {
      String s;
      jsonConfig.obj().printTo(s);
      server.send(200, "text/json", s);
    } else {
      server.send(200, "text/json", "[]");
    }
  });

  server.on("/json/getInfo", HTTP_GET, [](){
      String s = "{\"sketch\":\""+getThisSketch()+"\"}";
      server.send(200, "text/json", s);
  });

  // ----- FTP -----
  server.on("/ftpopen", HTTP_GET, [](){
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.open("192.168.1.100", "sadakane", "gorosan4252karatani");
    Ftp.cd("disk1/share/sadakane/FTP");
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });
  server.on("/ftpbye", HTTP_GET, [](){
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.bye();
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });
  server.on("/ftpls", HTTP_GET, [](){
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.ls();
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });
  server.on("/ftpcd", HTTP_GET, [](){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.cd(path);
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });
  server.on("/ftpget", HTTP_GET, [](){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.get(path);
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });
  server.on("/ftpput", HTTP_GET, [](){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    if( SPIFFS.exists("/tmp_ftp_client_log.txt") ) SPIFFS.remove("/tmp_ftp_client_log.txt");
    DebugOut.setToFile("/tmp_ftp_client_log.txt");
    Ftp.put(path);
    handleFileRead("/tmp_ftp_client_log.txt");
    DebugOut.setToPrevious();
  });


  // show file uploder
  server.on("/uploader", HTTP_GET, handleFileUploader);


  // others
  //   called when the url is not defined here. use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri())){
      if(server.uri()=="/" || server.uri()=="/index.htm") handleFileUploader(); //show uploader instead
      else server.send(404, "text/plain", "File Not Found");
    }
  });


  // ----------------- SERVER START ----------------------------------------------
  server.begin();
  DBG_OUTPUT_PORT.print("HTTP MyCockpit server started on port:");
  DBG_OUTPUT_PORT.println(SERVER_PORT_NO);
  DBG_OUTPUT_PORT.print("To move to Cockpit, open http://");
  DBG_OUTPUT_PORT.println(WiFi.localIP().toString());
}
 
void loopMyCockpit(void){
  server.handleClient();
}

