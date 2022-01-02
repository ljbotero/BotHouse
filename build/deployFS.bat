set nodemcu_spiffs="%UserProfile%\bot.local\build\nodemcu\ArduinoBotLocal.spiffs.bin"
set esp01_spiffs="%UserProfile%\bot.local\build\esp01\ArduinoBotLocal.spiffs.bin"
set espota=C:\Users\ljbot\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.6.2\tools\espota.py

REM Botones Alexa 192.168.2.245
%espota% -i 192.168.2.245 -p 8266 -s -f %nodemcu_spiffs%

REM Oficina
%espota% -i 192.168.2.220 -p 8266 -s -f %esp01_spiffs%

REM Upstairs-Mariana Bath Switch
%espota% -i 192.168.2.198 -p 8266 -s -f %nodemcu_spiffs%

REM Test 
%espota% -i 192.168.2.243 -p 8266 -s -f %nodemcu_spiffs%

REM TEST BUTTON
espota.py -i 192.168.2.81 -p 8266 -s -f %nodemcu_spiffs%
