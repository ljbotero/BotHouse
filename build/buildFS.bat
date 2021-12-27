REM https://github.com/esp8266/Arduino/issues/2320
REM -s = (spiffs_end - spiffs_start)

set mkspiffs=G:\My Drive\Projects\bot.local\build\mkspiffs.exe
set data="G:\My Drive\Projects\bot.local\Arduino\data"
set nodemcu_spiffs="G:\My Drive\Projects\bot.local\build\nodemcu\Arduino.spiffs.bin"
set esp01_spiffs="G:\My Drive\Projects\bot.local\build\esp01\Arduino.spiffs.bin"
set esp12s_spiffs="G:\My Drive\Projects\bot.local\build\esp12s\Arduino.spiffs.bin"

%mkspiffs% -p 256 -b 8192 -s 2072576 -c %data% %nodemcu_spiffs%
%mkspiffs% -p 256 -b 8192 -s 80000 -c %data% %esp01_spiffs%
%mkspiffs% -p 256 -b 8192 -s 2072576 -c %data% %esp12s_spiffs%