:: https://diyprojects.io/esp-easy-flash-firmware-esptool-py-esp8266-github/#.Ydof5WjMJHU
:: esptool.py --port [serial-port-of-ESP8266] write_flash -fm [mode] -fs [size] 0x00000 [nodemcu-firmware].bin
:: pip install esptool

set esptool=%UserProfile%\AppData\Local\Programs\Python\Python310\Lib\site-packages\esptool.py
set nodemcu="%UserProfile%\bot.local\build\nodemcu\Arduino.ino.bin"
set esp01="%UserProfile%\bot.local\build\esp01\Arduino.ino.bin"
set esp12s="%UserProfile%\bot.local\build\esp12s\Arduino.ino.bin"

:: %esptool% --port COM9 flash_id
%esptool% --chip esp8266 --port COM9 --baud 115200 --before no_reset --after hard_reset --connect-attempts 100 write_flash 0x00000 %esp12s% 
