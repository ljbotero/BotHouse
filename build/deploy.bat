set nodemcu_bin="%UserProfile%\bot.local\build\nodemcu\Arduino.ino.bin"
set esp01_bin="%UserProfile%\bot.local\build\esp01\Arduino.ino.bin"
set esp12s_bin="%UserProfile%\bot.local\build\esp12s\Arduino.ino.bin"
set espota=%UserProfile%\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.0.2\tools\espota.py

:: Oficina 02/21-3:00pm
:: %espota% -i 192.168.2.15 -p 8266 -f %esp12s_bin% 

:: Upstairs-Niños Bath Switch 02/21-8:00pm
:: %espota% -i 192.168.2.252 -p 8266 -f %esp12s_bin% 

:: Upstairs-Mariana Bath Switch 02/21-8:00pm
:: %espota% -i 192.168.2.227 -p 8266 -f %nodemcu_bin%
:: %espota% -i 192.168.2.227 -p 8266 -f %esp12s_bin%

:: Upstairs-Mariana Bath Relay 02/21-11:10am
:: %espota% -i 192.168.2.158 -p 8266 -f %esp12s_bin%

:: Downstairs bathroom light 02/21-8:00pm
:: %espota% -i 192.168.2.240 -p 8266 -f %esp12s_bin%

%espota% -i 192.168.2.160 -p 8266 -f %nodemcu_bin%