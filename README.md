# What is bot.house?
Bot.Local is a configurable IoT Framework running on ESP-based processors, like ESP-8266, ESP-12S, or ESP01. Multiple bot.house nodes can self-organize and create a mesh network where some of the nodes become stations (STA) and others become access point repeaters (AP) depending on their proximity and signal strength from each other. Each node can be configured to work as a switch or switches, a button or buttons, or switch relay. The later let's you control for instance a light switch.

Right now bot local supports Hubitat home automation Hub, which you can use to program different types of routines.

Bot.local does not require the internet to run, it is capable of keep working regardless of network connectivity.

# Mesh Architecture
Here's a sample of how multiple bot.house devices will form a self-organizing mesh, where each node will automatically determine if becoming an AP or STA depending on the signal strenght from each other. 

![Mesh Architecture](screenshots/mesh-architecture.png)

# Instructions

## Arduino
1. Choose your hardware. I recommend the ESP8266 if you have sufficient space as this is a quite big component, then you can add additional components, such as Relays, buttons, etc. If you are concerned about space, like if you want to install bot.house inside a light switch, you can order the following components: 
   - [ESP8266 5V WiFi Relay Module](https://amzn.to/36h0xXX) <details><summary>Show image</summary>![](screenshots/ESP8266WiFiRelayModule.PNG)</details>
   - [HLK-PM01 AC-DC 220V to 5V Step-Down Power Supply Module](https://amzn.to/39m590Q) <details><summary>Show image</summary>![](screenshots/HLK-PM01.PNG)</details>
1. Open the [Arduino folder](https://github.com/ljbotero/bot.house/tree/main/Arduino) with the the IDE of your choice (I like [Visual Studio Code](https://code.visualstudio.com/))
1. Open the /data folder and create your config files for the type of Hardware you are building. You might use the config_switch_relay.json as example to build a relay switch with the hardware listed above. Just make sure you rename the file name to config_[yourHawdwareId].json or just config.json.
1. Flash bot.house into your ESP hardware. I recommend using the Arduino IDE. Make sure you choose a flash with at least FS:512KB.
1. Flash the File System into the ESP hardware. There's an add in for Arduino IDE called "[ESP8266 Sketch Data Upload](https://github.com/esp8266/arduino-esp8266fs-plugin)" that I recommend for this purpose.
1. Setup your configuration by restarting your ESP hardware, then using your phone, connect to the Access Point with a name starting with botlocal. Once connected, open your browser and navigate to http://192.168.4.1. Setup your wifi name, password, and choose an iptional HUB, if you have one, then save changes.
1. Here's how it looks like once assembled and installed:
   ![](screenshots/esp01-relay.png)

## Hubitat
1. From the Hubitat admin site, go to "Apps Code", then upload [Hubitat/bot-local-hubitat.groovy](Hubitat/bot-local-hubitat.groovy), save it, 
1. Click on OAuth button on top and enable it.
1. Navigate to Apps section, and click on [Add User App], choose BotHouse
1. Then the app will launch and start discovering any devices already deployed.
1. Once all devices have been found you can click the [Done] button.

# Admin UI

Admin UI allows administering all nodes in the Mesh. If the mesh is connected to your Wifi, you can access the mesh UI via http://bot.local  
If not connected to your WiFi, you might connect to one of the access points in the mesh, named as LocalBotXYZ, then open the following in your browser: http://192.168.4.1

![Home-configuration](screenshots/home-configuration.png)

![Home-devices](screenshots/home-devices.png)

![Device-configuration](screenshots/Device-configuration.png)