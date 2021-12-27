set nodemcu_bin="G:\My Drive\Projects\bot.local\build\nodemcu\Arduino.ino.bin"
set esp01_bin="G:\My Drive\Projects\bot.local\build\esp01\Arduino.ino.bin"
set esp12s_bin="G:\My Drive\Projects\bot.local\build\esp12s\Arduino.ino.bin"
set espota=C:\Users\ljbot\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.0.2\tools\espota.py

REM Oficina 02/21-3:00pm
%espota% -i 192.168.2.15 -p 8266 -f %esp12s_bin% 

REM Upstairs-Niños Bath Switch 02/21-8:00pm
%espota% -i 192.168.2.252 -p 8266 -f %esp12s_bin% 

REM Upstairs-Mariana Bath Switch 02/21-8:00pm
REM %espota% -i 192.168.2.227 -p 8266 -f %nodemcu_bin%
%espota% -i 192.168.2.227 -p 8266 -f %esp12s_bin%

REM Upstairs-Mariana Bath Relay 02/21-11:10am
%espota% -i 192.168.2.81 -p 8266 -f %esp12s_bin%

REM Downstairs bathroom light 02/21-8:00pm
%espota% -i 192.168.2.240 -p 8266 -f %esp12s_bin%
REM %espota% -i 192.168.4.100 -p 8266 -f %esp12s_bin%
