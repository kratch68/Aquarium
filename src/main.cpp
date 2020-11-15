#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <Wire.h>

AsyncWebServer server(80);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid="helix_88";
const char* password="maison88-2020";


const char* PARAM_NOMAQUARIUM = "nomAquarium";
String inputMessage;

// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang='fr'>
	<head>
	  <title>ESP Input Form</title>
	  <meta name="viewport" content="width=device-width, initial-scale=1" charset='UTF-8'/>
	  <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>
	  <script>
		function submitMessage() {
		  alert("Saved value to ESP SPIFFS");
		  setTimeout(function(){ document.location.reload(false); }, 500);   
		}
	  </script>
	</head>
	<header>
        <div class="w3-container w3-light-grey w3-center">
            <h1>Aquarium</h1>
        </div>
    </header>
	<body>
        <main class="w3-light-grey" style="width: 100%; height: 350px; display: flex;">
            <div style="width: 50%; text-align: center;">
                <form action="/get" target="hidden-form">
                    <h1>Modifier le nom : %nomAquarium% </h1>
                    <input class='w3-input w3-round-large' type='text' name="nomAquarium" style="width: 50%; margin: auto;" placeholder='Nom aquarium'>
                    <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Modifier</button>
                </form>
            </div>
        </main>
    </body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

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
  //Serial.println(var);
  if(var == "nomAquarium"){
    return readFile(SPIFFS, "/nomAquarium.txt");
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_NOMAQUARIUM)) {
      inputMessage = request->getParam(PARAM_NOMAQUARIUM)->value();
      writeFile(SPIFFS, "/nomAquarium.txt", inputMessage.c_str());
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

void Oled()
{

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  String nomAquarium = readFile(SPIFFS, "/nomAquarium.txt");

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(nomAquarium);
  display.print("IP: ");
  display.print(WiFi.localIP());
  display.display();
}

void loop() {
  // To access your stored values on inputString, inputInt, inputFloat
  String yourInputString = readFile(SPIFFS, "/nomAquarium.txt");
  Serial.print("*** Your nomAquarium: ");
  Serial.println(yourInputString);
  Oled();
  delay(5000);
}