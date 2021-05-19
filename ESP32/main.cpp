#include <Arduino.h> //Duh
#include <WiFi.h> // Til Wifi
#include <AsyncTCP.h> // Webserver
#include <SPIFFS.h> // Fil tingting
#include <ESPAsyncWebServer.h> // Webserver

#define statusLED 27 // Lyser hvis alt er godt

AsyncWebServer server(80); // Port 80

// Netværksoplysninger
const char* ssid = "Kelvin2";
const char* password = "Celsius16";
// Variable i HTML blokken
const char* PARAM_STRING = "lilleInput";
const char* PARAM_INT = "mellemInput";
const char* PARAM_FLOAT = "storInput";
const char* PARAM_SEND = "sendKnap";

// HTML blok
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Bestillingssystem</title>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
  article { display: inline-grid; grid-template-rows: auto auto auto; grid-template-columns: auto auto auto; grid-gap: 3px;}
  section { display: flex; justify-content: center; align-items: center; border: 3px solid #bb8082; }
  body {background-color: #f39189; color: white;}
  input[type=submit] {
    padding:5px 15px; 
    background:#046582;
    color: white; 
    border:0 none;
    cursor:pointer;
    -webkit-border-radius: 5px;
    border-radius: 5px; 
  }
  text-decoration: none; color: white; font-size: 30px; margin: 2px; cursor: pointer;}");
  
  </style>
    <script>
      function submitMessage() {
        setTimeout(function(){ document.location.reload(false); }, 500);   
      }
  </script></head>
  <body>
    <h1>Bestillingssystem</h1>

    <article>
    <section><h3>Lille Pils</h3></section>
    <section><h3>Medium Pils</h3></section>
    <section><h3>Stor Pils</h3></section>

    <section><img src='https://1b5zut3265xakph9m2yvnc4z-wpengine.netdna-ssl.com/wp-content/uploads/2020/05/1021_HB_PIL_33_bottle_DK_960x960.png' id='billede' alt='Memes' width='200' height='200'></section>
    <section><img src='https://1b5zut3265xakph9m2yvnc4z-wpengine.netdna-ssl.com/wp-content/uploads/2020/05/1021_HB_PIL_33_bottle_DK_960x960.png' id='billede' alt='Memes' width='200' height='200'></section>
    <section><img src='https://1b5zut3265xakph9m2yvnc4z-wpengine.netdna-ssl.com/wp-content/uploads/2020/05/1021_HB_PIL_33_bottle_DK_960x960.png' id='billede' alt='Memes' width='200' height='200'></section>
                
    <section><form action="/get" target="hidden-form">
      Antal (nuværende: %lilleInput%):  <input type="number" name="lilleInput"> <!-- Variable i HTML'en -->
      <input type="submit" value="Gem" onclick="submitMessage()">
    </form></section>
    <section><form action="/get" target="hidden-form">
      Antal: (nuværende: %mellemInput%)<input type="number" name="mellemInput"> <!-- Variable i HTML'en -->
      <input type="submit" value="Gem" onclick="submitMessage()">
    </form></section>
    <section><form action="/get" target="hidden-form">
      Antal: (nuværende: %storInput%)<input type="number" name="storInput"> <!-- Variable i HTML'en -->
      <input type="submit" value="Gem" onclick="submitMessage()">
    </form></section>
    <section> </section>
    <section><form action="/get" target="hidden-form">
      <!-- Antal: (nuværende: %sendKnap%)<input type="number" name="sendKnap"> --> <!-- Variable i HTML'en -->
      <input type="hidden" name="sendKnap" value="1">
      <input type="submit" value="Send" onclick="submitMessage()">
    </form></section>
    <section> </section>
    <iframe style="display:none" name="hidden-form"></iframe></article>
  </body></html>)rawliteral";

// 404 Handler
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Læser fra de 3 lokale file
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}
// Skriver til de 3 lokale filer
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String& var){
  if(var == "lilleInput"){
    return readFile(SPIFFS, "/lilleInput.txt");
  }
  else if(var == "mellemInput"){
    return readFile(SPIFFS, "/mellemInput.txt");
  }
  else if(var == "storInput"){
    return readFile(SPIFFS, "/storInput.txt");
  }
  return String();
}

void setup() {
  pinMode(statusLED, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(115200);
  // Initialize SPIFFS

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Wifi init
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(statusLED, HIGH);

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_STRING)) {
      inputMessage = request->getParam(PARAM_STRING)->value();
      int lilleInputPrint = readFile(SPIFFS, "/lilleInput.txt").toInt();
      if (((inputMessage.toInt() + lilleInputPrint) > 0) && ((inputMessage.toInt() + lilleInputPrint) < 16)){
        String tempLille = String(inputMessage.toInt() + lilleInputPrint);
        writeFile(SPIFFS, "/lilleInput.txt", tempLille.c_str());
      } else if ((inputMessage.toInt() + lilleInputPrint) <= 0) {
        writeFile(SPIFFS, "/lilleInput.txt", String("0").c_str());
      } else {
        writeFile(SPIFFS, "/lilleInput.txt", String("15").c_str());
      }
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      int mellemInputPrint = readFile(SPIFFS, "/mellemInput.txt").toInt();
      if (((inputMessage.toInt() + mellemInputPrint) > 0) && ((inputMessage.toInt() + mellemInputPrint) < 16)){
        String tempMellem = String(inputMessage.toInt() + mellemInputPrint);
        writeFile(SPIFFS, "/mellemInput.txt", tempMellem.c_str());
      } else if ((inputMessage.toInt() + mellemInputPrint) <= 0) {
        writeFile(SPIFFS, "/mellemInput.txt", String("0").c_str());
      } else {
        writeFile(SPIFFS, "/mellemInput.txt", String("15").c_str());
      }
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT)) {
      inputMessage = request->getParam(PARAM_FLOAT)->value();
      int storInputPrint = readFile(SPIFFS, "/storInput.txt").toInt();
      if (((inputMessage.toInt() + storInputPrint) > 0) && ((inputMessage.toInt() + storInputPrint) < 16)){
        String tempStor = String(inputMessage.toInt() + storInputPrint);
        writeFile(SPIFFS, "/storInput.txt", tempStor.c_str());
      } else if ((inputMessage.toInt() + storInputPrint) <= 0) {
        writeFile(SPIFFS, "/storInput.txt", String("0").c_str());
      } else {
        writeFile(SPIFFS, "/storInput.txt", String("15").c_str());
      }
    }
    // GET inputInt value on <ESP_IP>/get?sendKnap=<inputMessage>
    else if (request->hasParam(PARAM_SEND)) {
      //inputMessage = request->getParam(PARAM_INT)->value();
      if ((request->getParam(PARAM_SEND)->value()).toInt() == 1){
        int lilleInputPrint = readFile(SPIFFS, "/lilleInput.txt").toInt();
        int mellemInputPrint = readFile(SPIFFS, "/mellemInput.txt").toInt();
        int storInputPrint = readFile(SPIFFS, "/storInput.txt").toInt();
        if (lilleInputPrint > 0){ // Besked opbygning og afsending
          byte pils = B01000000; //Pils identifier
          byte stoerrelse = B00010000; // Size identifier
          byte antal = byte(lilleInputPrint); // Amount
          byte done = pils + stoerrelse + antal; // Added together to the format Pils-Size-Amount ex: 01010001
          Serial1.write(done); // Send it
        }
        if (mellemInputPrint > 0){ // Besked opbygning og afsending
          byte pils = B01000000; //Pils identifier
          byte stoerrelse = B00100000; // Size identifier
          byte antal = byte(mellemInputPrint); // Amount
          byte done = pils + stoerrelse + antal; // Added together to the format Pils-Size-Amount ex: 01100001
          Serial1.write(done); // Send it
        }
        if (storInputPrint > 0){ // Besked opbygning og afsending
          byte pils = B01000000; //Pils identifier
          byte stoerrelse = B00110000; // Size identifier
          byte antal = byte(storInputPrint); // Amount
          byte done = pils + stoerrelse + antal; // Added together to the format Pils-Size-Amount ex: 01110001
          Serial1.write(done); // Send it
        }
        // Reset save files.
        writeFile(SPIFFS, "/lilleInput.txt", String(0).c_str()); 
        writeFile(SPIFFS, "/mellemInput.txt", String(0).c_str());
        writeFile(SPIFFS, "/storInput.txt", String(0).c_str());
      }
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
}
