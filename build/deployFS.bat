set nodemcu_spiffs="%UserProfile%\bot.local\build\nodemcu\Arduino.spiffs.bin"
set nodemcu_spiffs="%UserProfile%\bot.local\build\nodemcu\Arduino.spiffs.bin"
set esp01_spiffs="%UserProfile%\bot.local\build\esp01\Arduino.spiffs.bin"
set espota="%UserProfile%\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.0.2\tools\espota.py"

:: Botones Alexa 192.168.2.245
:: %espota% -i 192.168.2.245 -p 8266 -s -f %nodemcu_spiffs%

:: Oficina
:: %espota% -i 192.168.2.220 -p 8266 -s -f %esp01_spiffs%

:: Upstairs-Mariana Bath Switch
:: %espota% -i 192.168.2.198 -p 8266 -s -f %nodemcu_spiffs%

:: Test 
:: %espota% -i 192.168.2.243 -p 8266 -s -f %nodemcu_spiffs%

:: TEST BUTTON
:: %espota% -i 192.168.2.81 -p 8266 -s -f %nodemcu_spiffs%

%espota% -i 192.168.2.160 -p 8266 -s -f %nodemcu_spiffs%
