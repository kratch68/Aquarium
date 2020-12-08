/*
* TP: Aquarium
* Nom: main.cpp
* But: Permet l'execution et le fonctionnement du systeme embarquer de l'aquarium
* Createur: Cabellic gwenael
* Date: 08/12/2020
*/

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
#include <time.h>

//Initialisation du serveur
AsyncWebServer server(80);

//Initaialisation du Oled
#define screen_width 128
#define screen_height 64
Adafruit_SSD1306 display(screen_width, screen_height, &Wire, -1);

//initialisation de la sonde de temperature
#define water_temp_pin 15
OneWire oneWire(water_temp_pin); 
DallasTemperature sensors(&oneWire);

//Initialisation du timer
time_t now;

//LED pour Chauffage
const int ledChauffage = 2;

//Pompe
const int pompe = 13;

//Connexion au wifi
const char* SSID="maison_88";
const char* PASSWORD="asticot008";

//Parametre pour chaque parametre utile a l'application
const char* PARAM_NOMAQUARIUM = "nomAquarium";
const char* PARAM_TEMPMIN = "tempMin";
const char* PARAM_TEMPMAX = "tempMax";
const char* PARAM_FREQPOMPE = "freqPompe";
const char* PARAM_TempsPompe = "tempPompe";

//Page WEB en Html
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang='fr'>
	<head>
	  <title>ESP Input Form</title>
	  <meta name="viewport" content="width=device-width, initial-scale=1" charset='UTF-8'/>
	  <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css">
	  <script>
		function submitMessage() {
		  alert("Saved value to ESP SPIFFS");
		  setTimeout(function(){ document.location.reload(false); }, 500);   
		}
	  </script>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js"></script>
    <script type="text/javascript">
      function googleTranslateElementInit() {
        new google.translate.TranslateElement({pageLanguage: 'en'}, 'google_translate_element');
      }
    </script>
  <script type="text/javascript" src="//translate.google.com/translate_a/element.js?cb=googleTranslateElementInit"></script>
	</head>
	<header>
        <div class="w3-container w3-light-grey w3-center">
            <h1>Aquarium</h1>
        </div>
        
    </header>
	<body>
        <div id="google_translate_element" class=" w3-light-grey"></div>
        <main class=" w3-light-grey" style="width: 1920px; height: 200px;">
            <div style="width: 800px; text-align: center; margin: auto;">
            <h1>Modifier le nom : %nomAquarium% </h1>
                <form action="/get" target="hidden-form" style="display: flex;">
                    <input class='w3-input w3-round-large' type='text' name="nomAquarium" style="width: 500px; margin: auto;" placeholder="Nom aquarium">
                    <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Modifier</button>
                </form>
            </div>
        </main>
        <main class=" w3-light-grey" style="width: 1920px; height: 200px;">
          <div style="width: 900px; text-align: center; margin: auto;">
            <h1>Temperature : </h1>
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
        <main class="bas w3-light-grey" style="width: 1920px; height: 250px;">
          <div style="width: 750px; text-align: center; margin: auto;">
            <h1>Pompe :</h1>
            <form action="/get" target="hidden-form" style="display: flex;">
              Frequence de la pompe : %freqPompe% min :<select class="w3-select w3-round-large" id="freqPompe" name="freqPompe" style="width: 300px; margin: auto;">
                  <option value="" disabled selected>choisie ton option</option>
                  <option value="1">1min</option>
                  <option value="2">2min</option>
                  <option value="5">5min</option>
                  </select>
              <button onclick="submitMessage()" type="submit" value="Submit" class='w3-btn w3-white w3-border w3-round-large'>Sauvegarder</button>
            </form>
            <form action="/get" target="hidden-form" style="display: flex;">
              Temps de fonctionnement : %tempPompe% min :<select class="w3-select w3-round-large" id="tempPompe" name="tempPompe" style="width: 300px; margin: auto;">
                  <option value="" disabled selected>choisie ton option</option>
                  <option value="1">1min</option>
                  <option value="2">2min</option>
                  <option value="5">5min</option>
                  </select>
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

//Function permettant de lire les valeurs dans leur fichier dans le SPIFFS
String readFile(fs::FS &fs, const char * path){
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  return fileContent;
}

//Function permettant d'ecrire dans les fichiers dans le SPIFFS
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

// Function qui permet de remplacer les holder avec les valeurs
String processor(const String& var){
  if(var == "nomAquarium"){
    return readFile(SPIFFS, "/nomAquarium.txt");
  }
  else if(var == "tempMin"){
    return readFile(SPIFFS, "/tempMin.txt");
  }
  else if(var == "tempMax"){
    return readFile(SPIFFS, "/tempMax.txt");
  }
  else if(var == "freqPompe"){
    return readFile(SPIFFS, "/freqPompe.txt");
  }
  else if(var == "tempPompe"){
    return readFile(SPIFFS, "/tempPompe.txt");
  }
  return String();
}

void setup() {
  Serial.begin(115200);
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

  //Initialisation du WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin("maison88","asticot008");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  //retourne l'adresse IP
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  //Permet d'afficher la page Web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  //Permet de recuperer les valeurs dans les input
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
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
    else if (request->hasParam(PARAM_FREQPOMPE)) {
      inputMessage = request->getParam(PARAM_FREQPOMPE)->value();
      writeFile(SPIFFS, "/freqPompe.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TempsPompe)) {
      inputMessage = request->getParam(PARAM_TempsPompe)->value();
      writeFile(SPIFFS, "/tempPompe.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  pinMode(ledChauffage, OUTPUT);
  pinMode(pompe, OUTPUT);
  server.onNotFound(notFound);
  server.begin();
}

//Function qui permet de recuperer la temperature
double getTemperature()
{
  sensors.requestTemperatures();
  double temperatureActuelle = sensors.getTempCByIndex(0);
  return temperatureActuelle;
  delay(1000);
}

//Function qui permet la regulation du chauffage
void chauffage()
{
  int tempMin = readFile(SPIFFS, "/tempMin.txt").toInt();
  int tempMax = readFile(SPIFFS, "/tempMax.txt").toInt();
  double temperatureActuelle = getTemperature();
  int chauffage;

  if(temperatureActuelle<=tempMin)
    {
      digitalWrite(ledChauffage,HIGH);
      chauffage = 1;
    }
  else if (temperatureActuelle<=tempMax && chauffage == 1)
  { 
    digitalWrite(ledChauffage,HIGH);
  }
  else if(temperatureActuelle>=tempMax)
    {
      digitalWrite(ledChauffage,LOW);
      chauffage = 0;
    }
}

//Function qui permet l'affichage des information sur le OLED
void oled()
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  String nomAquarium = readFile(SPIFFS, "/nomAquarium.txt");
  int tempMin = readFile(SPIFFS, "/tempMin.txt").toInt();
  int tempMax = readFile(SPIFFS, "/tempMax.txt").toInt();
  int freqPompe = readFile(SPIFFS, "/freqPompe.txt").toInt();
  int tempPompe = readFile(SPIFFS, "/tempPompe.txt").toInt();
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
  display.println(tempMax);
  display.println("Pompe: ");
  display.print("Frequence: ");
  display.print(freqPompe);
  display.println(" min");
  display.print("Temps: ");
  display.print(tempPompe);
  display.println(" min");
  display.display();
}

//Permet d'obtenir la duree de fonctionnement de la pompe
int obtenirPompeDuree()
{
  int tempPompe = readFile(SPIFFS, "/tempPompe.txt").toInt();
  return tempPompe;
}

//permet d'obtenir le temps de frequence de la pompe
int obtenirPompeFreq()
{
  int freqPompe = readFile(SPIFFS, "/freqPompe.txt").toInt();
  return freqPompe;
}

//Permet le fontionnement de la pompe
void fonctionnementPompe()
{
  if (digitalRead(pompe) == LOW){
      if((time(&now) % (obtenirPompeDuree() * 30)) == 0)
          {
            digitalWrite(pompe, HIGH);
            delay(1000);
          }}
if (digitalRead(pompe) == HIGH){
       if((time(&now) % (obtenirPompeFreq() * 30)) == 0)
          {
            digitalWrite(pompe, LOW);
            delay(1000);
          }}
}

void loop() {
  //Permet d'afficher les informations dans le moniteur serie
  /*String nomAquarium = readFile(SPIFFS, "/nomAquarium.txt");
  int tempMin = readFile(SPIFFS, "/tempMin.txt").toInt();
  int tempMax = readFile(SPIFFS, "/tempMax.txt").toInt();
  int freqPompe = readFile(SPIFFS, "/freqPompe.txt").toInt();
  int tempPompe = readFile(SPIFFS, "/tempPompe.txt").toInt();
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
  Serial.print("Frequence Pompe: ");
  Serial.println(freqPompe);
  Serial.print("Temps Pompe: ");
  Serial.println(tempPompe);*/
  chauffage();
  oled();
  fonctionnementPompe();
  delay(250);
}