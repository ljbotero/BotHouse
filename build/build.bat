REM https://github.com/arduino/Arduino/blob/master/build/shared/manpage.adoc
REM C:\Users\ljbot\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.6.2\boards.txt

set arduino="C:\Program Files (x86)\Arduino\arduino_debug.exe"
set ino="G:\My Drive\Projects\bot.local\Arduino\Arduino.ino"
set nodemcu="G:\My Drive\Projects\bot.local\build\nodemcu"
set esp01="G:\My Drive\Projects\bot.local\build\esp01"
set esp12s="G:\My Drive\Projects\bot.local\build\esp12s"

REM %arduino% --verify --board esp8266:esp8266:nodemcuv2 --preserve-temp-files --pref build.path=%nodemcu% %ino%
REM %arduino% --verify --board esp8266:esp8266:generic:eesz=1M64 --preserve-temp-files --pref build.path="%esp01% %ino%
%arduino% --verify --board esp8266:esp8266:espino:eesz=4M2M --preserve-temp-files --pref build.path=%esp12s% %ino%
