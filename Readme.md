# ESP D1 mini Smat steckdose
[![en](https://img.shields.io/badge/lang-en-red.svg)](https://github.com/Protokollmaker/ESP_Steckdose/blob/master/Readme.en.md)
[![de](https://img.shields.io/badge/lang-de-green.svg)](https://github.com/Protokollmaker/ESP_Steckdose/blob/master/Readme.md)
## Ziehl des pojectes
**Das ist ein Schuleproject** </br>
Das Ziel dieses Projectes ist Steckdosen von einen Web interface zu steuern.</br>
Hier ein Bild: </br>
<img src="https://github.com/Protokollmaker/ESP_Steckdose/blob/master/docs/image/AppImage.png?raw=true" alt="applicaionen Bild">
## Get Startet
### Hardware
1. [Materialien die Benötigt werden](https://github.com/Protokollmaker/ESP_Steckdose/blob/master/docs/matherial.md)
1. []()
### Software
1. Downlade [Arduino ide](https://www.arduino.cc/en/software)
1. Install Lib manuel [🔗Install LittleLS](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html) </br> Wichtig: es muss kein upload Butten da sein, dies geht mit IDE 2.X noch nicht.
1. Install Lib [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) von me-no-dev
1. Erstelle einen File in backend/main mit den namen password.h </br> Füge hier: </br> `#define SSID_WLAN "Netzwerkname"`</br>`#define PASSWORD_WLAN "Netzwerk password"`</br>ein
1. upload skatch zu Microcontroller
1. Öffne serial Console und notiere die IP Adresse die angezeigt wird, wenn keine angezeigt wird drücke den Reset Knopf auf den Borad
1. gebe in den Browser http://{deine IP}/ hier soltest du die Website wie oben sehen

## TODO

### Ideen
- Hinzufügen von Zeiteingabe
- Timer die sich widerholen
- dark thema