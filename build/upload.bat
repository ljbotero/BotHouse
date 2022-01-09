REM https://diyprojects.io/esp-easy-flash-firmware-esptool-py-esp8266-github/#.Ydof5WjMJHU
REM esptool.py --port [serial-port-of-ESP8266] write_flash -fm [mode] -fs [size] 0x00000 [nodemcu-firmware].bin
REM pip install esptool

set espota=%UserProfile%\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.0.2\tools\esptool\esptool.py
set nodemcu="%UserProfile%\bot.local\build\nodemcu\Arduino.ino.bin"
set esp01="%UserProfile%\bot.local\build\esp01\Arduino.ino.bin"
set esp12s="%UserProfile%\bot.local\build\esp12s\Arduino.ino.bin"

REM %arduino% --verify --board esp8266:esp8266:nodemcuv2 --preserve-temp-files --pref build.path=%nodemcu% %ino%
REM %arduino% --verify --board esp8266:esp8266:generic:eesz=1M64 --preserve-temp-files --pref build.path="%esp01% %ino%

%espota%  --port COM9 --baud 115200 write_flash -fm dio -fs 4MB 0x00000 %esp12s% 
