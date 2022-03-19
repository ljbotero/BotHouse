set nodemcu_bin="%UserProfile%\bot.local\build\nodemcu\Arduino.ino.bin"
set esp01_bin="%UserProfile%\bot.local\build\esp01\Arduino.ino.bin"
set esp12s_bin="%UserProfile%\bot.local\build\esp12s\Arduino.ino.bin"
set espota=%UserProfile%\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.0.2\tools\espota.py

:: Oficina 02/21-3:00pm
:: %espota% -i 192.168.2.15 -p 8266 -f %esp12s_bin% 

:: Upstairs-Niños Bath Switch 02/21-8:00pm
:: %espota% -i 192.168.2.252 -p 8266 -f %esp12s_bin% 

:: Upstairs-Mariana Bath Relay - 02/13/2022-7:50am
%espota% -i 192.168.2.157 -p 8266 -f %esp12s_bin%

:: Upstairs-Mariana Bath Switch 02/21-8:00pm
%espota% -i 192.168.2.175 -p 8266 -f %nodemcu_bin%

:: 72DCAB-Upstairs-Guest - 02/13/2022-7:50am
::%espota% -i 192.168.2.150 -p 8266 -f %esp12s_bin%

:: Motion sensor - 02/13/2022-7:50am
%espota% -i 192.168.2.121 -p 8266 -f %nodemcu_bin%

:: Flow Sensor - 02/13/2022-7:50am
%espota% -i 192.168.2.162 -p 8266 -f %nodemcu_bin%