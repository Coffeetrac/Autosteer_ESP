Currently the Arduino IDE V1.8.8 causes problems writing the EEprom of the ESP.               
Please use 1.8.7 instead, until i find a solution.

(If you already failed with the 1.8.8 you have to change the EEIdent2 byte a little, maybe from 0xED to 0xEC              
in the file misc.ino, this initiates rewriting the bad EEprom data on first start)



The ESP32 enhancement of the arduino IDE works since 2019 JAN.04 with both Ways:

The easy version with the boardmanager:                                                            
 https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md


and also with the more tricky version over gitgui:                                                             
 https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/windows.md




OLED LIBRARY:                                                                  
You can find the used SSD1306 in the LibraryManager of the Arduino IDE, it's called:
ESP8266 and ESP32 Oled Driver for SSD1306 display by Daniel Eichhorn, Fabrice Weinberg


GOOD LUCK
