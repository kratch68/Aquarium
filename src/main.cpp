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
#include <OneWire.h>
#include <DallasTemperature.h>


AsyncWebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define WATER_TEMP_PIN 15
OneWire oneWire(WATER_TEMP_PIN); 
DallasTemperature sensors(&oneWire);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid="helix_88";
const char* password="maison88-2020";


const char* PARAM_NOMAQUARIUM = "nomAquarium";
const char* PARAM_TEMPMIN = "tempMin";
const char* PARAM_TEMPMAX = "tempMax";

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
        <main class="haut w3-light-grey" style="width: 1920px; height: 150px;">
            <div style="width: 800px; text-align: center; margin: auto;">
            <h1>Modifier le nom : %nomAquarium% </h1>
                <form action="/get" target="hidden-form" style="display: flex;">
                    <input class='w3-input w3-round-large' type='text' name="nomAquarium" style="width: 500px; margin: auto;" placeholder="Nom aquarium">
                    <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Modifier</button>
                </form>
            </div>
        </main>
        <main class="bas w3-light-grey" style="width: 1920px; height: 350px;">
          <div style="width: 900px; text-align: center; margin: auto;">
            <h1>Temperature : %tempMin% </h1>
            <form action="/get" target="hidden-form" style="display: flex;">
              Temperature mini : %tempMin% °C :<input class='w3-input w3-round-large' type='number' name="tempMin" style="width: 500px; margin: auto;">
              <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Sauvegarder</button>
            </form>
            <form action="/get" target="hidden-form" style="display: flex;">
              Temperature max : %tempMax% °C :<input class='w3-input w3-round-large' type='number' name="tempMax" style="width: 500px; margin: auto;">
              <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Sauvegarder</button>
            </form>
          </div>
        </main>
        <iframe style="display:none" name="hidden-form"></iframe>
    </body>
</html>
)rawliteral";

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
  else if(var == "tempMin"){
    return readFile(SPIFFS, "/tempMin.txt");
  }
  else if(var == "tempMax"){
    return readFile(SPIFFS, "/tempMax.txt");
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
    String inputMessage;
    if (request->hasParam(PARAM_NOMAQUARIUM)) {
      inputMessage = request->getParam(PARAM_NOMAQUARIUM)->value();
      writeFile(SPIFFS, "/nomAquarium.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TEMPMIN)) {
      inputMessage = request->getParam(PARAM_TEMPMIN)->value();
      writeFile(SPIFFS, "/tempMin.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TEMPMAX)) {
      inputMessage = request->getParam(PARAM_TEMPMAX)->value();
      writeFile(SPIFFS, "/tempMax.txt", inputMessage.c_str());
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

double getTemperature()
{
  sensors.requestTemperatures();
  double temperatureActuelle = sensors.getTempCByIndex(0);
  return temperatureActuelle;

  delay(1000);
}

void Oled()
{

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  String nomAquarium = readFile(SPIFFS, "/nomAquarium.txt");
  int tempMin = readFile(SPIFFS, "/tempMin.txt").toInt();
  int tempMax = readFile(SPIFFS, "/tempMax.txt").toInt();
  double temperatureActuelle = getTemperature();

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(nomAquarium);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.print("Temperature: ");
  display.println(temperatureActuelle);
  display.print("Min: ");
  display.print(tempMin);
  display.print("   Max: ");
  display.print(tempMax);
  display.display();
}

void loop() {
  String nomAquarium = readFile(SPIFFS, "/nomAquarium.txt");
  int tempMin = readFile(SPIFFS, "/tempMin.txt").toInt();
  int tempMax = readFile(SPIFFS, "/tempMax.txt").toInt();
  double temperatureActuelle = getTemperature();
  Serial.print("Nom de l'aquarium: ");
  Serial.println(nomAquarium);
  Serial.print("Temperature de l'eau: ");
  Serial.print(temperatureActuelle);
  Serial.println("°C");
  Serial.print("Temperature Min: ");
  Serial.print(tempMin);
  Serial.println("°C");
  Serial.print("Temperature Max: ");
  Serial.print(tempMax);
  Serial.println("°C");
  Oled();
  delay(2000);
}