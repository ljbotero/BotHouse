/**
 *
 *  Copyright 2019
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 *  Flows:
 *  - Install: mainPage [Find Device] 
 *              -> pageDiscovery 
 *              -> discoverySearch [subscribe to ssdp events, discover devices]
 *                 -> discoverySearchHandler
 *                 -> discoveryVerify [POST /description.xml {apiDetails & authToken} ]
 *                      -> discoveryVerificationHandler
 *                 [device GET] -> getHubDetails [ device verified ]
 */


public static String version() { return "1.0.0" }

definition(
  name:        "${appName()}",
  namespace:   "${namespace()}",
  author:      "Jaime Botero",
  description: "${appName()} devices bridge wired things with ${hubNamespace()}",
  iconUrl:     "http://cdn.device-icons.smartthings.com/Home/home5-icn.png",
  iconX2Url:   "http://cdn.device-icons.smartthings.com/Home/home5-icn@2x.png",
  iconX3Url:   "http://cdn.device-icons.smartthings.com/Home/home5-icn@3x.png",
  category:    "Lights & Switches",
  singleInstance: true
)

mappings {
  //path("/device") { action: [ PUT: "addNewDevice"] }
  path("/deviceEvent") { action: [ POST: "handleDeviceEvent"] }
  path("/heartbeat") { action: [ POST: "handleDeviceHeartbeat"] }
  //path("/hub") { action: [ GET: "getHubDetails"] }
}

preferences {
  page(name: "mainPage", content: "mainPage") 
}

def installed() {
  logInfo("Installing ${appName()}")
  if (!state.isHubRegistered) {
    runEvery1Minute(discoverySearch)
  } else {
    runEvery30Minutes(discoverySearch)
  }
}

def uninstalled() {
  getAllChildDevices().each {
    log.debug "deletedChildDevice: ${it}"
    deleteChildDevice(it.deviceNetworkId)
  }
  state.device = null
  state.isHubRegistered = false;
  logInfo("Uninstalled ${appName()}")
}

def updated() {
  discoverySearch()
  if (!state.isHubRegistered) {
    runEvery1Minute(discoverySearch)
  } else {
    runEvery30Minutes(discoverySearch)
  }
  logInfo("Updated ${appName()}")
}

// User Interfaces
//**************************************************************
def mainPage(){
  discoverySearch()
  dynamicPage(name: "mainPage", install: true, uninstall: true, refreshInterval: 10) {
      if (state.isHubRegistered) {
        section {
            paragraph "${appName()} App v${version()} successfully registered."
            paragraph "Found the following devices:"
            getAllChildDevices().each {
              paragraph "• ${it.displayName}"
              log.debug "DEVICE INFO: ${it}"
              log.debug " supportedAttributes: ${it.supportedAttributes}" // [doubleTapped, held, numberOfButtons, pushed, released]
              log.debug " supportedCommands: ${it.supportedCommands}" // [doubleTap, hold, push, release]
              log.debug " capabilities: ${it.capabilities}"  // [HoldableButton, PushableButton, DoubleTapableButton, ReleasableButton]
            }
        }
      } else {
        section {
            paragraph "${appName()} App v${version()}"
            paragraph "Searching for devices, please wait..."
        }
      }
      if (state.info != null) {
        paragraph state.info
      }
      if (state.lastError != null)
      {        
        paragraph "ERROR: ${state.lastError}"
      }
  }
}

// Simple Discovery Protocol
//**************************************************************

def discoverySearch() {
  state.lastError = null
  state.info = null
  if (!state.subscribed) {
    log.debug "discoverySubscribe"
    subscribe(location, "ssdpTerm.${discoveryDeviceType()}", discoverySearchHandler, [filterEvents:false])
    state.subscribed = true
  }
  log.debug "1. Discovering devices..."
  sendHubCommand(new hubitat.device.HubAction("lan discovery ${discoveryDeviceType()}", hubitat.device.Protocol.LAN))
}

def discoverySearchHandler(evt) {
  def device = parseLanMessage(evt.description)
  String host = getDeviceIpAndPort(device)
  state.masterNodeIP = host;
  log.debug "masterNodeIP=${host}"
  def deviceChild = getChildDevice(device.ssdpUSN)
  if (deviceChild){ 
    log.debug "2. Device already registered ${device}"
    return
  }
  log.debug "2. Requesting Device Details ${host}${device.ssdpPath}"
  sendHubCommand(new hubitat.device.HubAction([
    method: "GET",
    path: device.ssdpPath,
    headers: [ HOST: host, "Accept": "application/json"]
  ], host, [callback: discoveryVerificationHandler]))
}


def discoveryVerificationHandler(hubitat.device.HubResponse hubResponse) {
  log.debug "3. Device responded with details"  

  addNewDevice(hubResponse)
  //if (!state.isHubRegistered){
  registerHubIntoDevice(hubResponse)
  //}
}

def addNewDevice(hubitat.device.HubResponse hubResponse){
  def body = hubResponse.xml
  def deviceInfo = body?.device
  def deviceId = deviceInfo?.UDN?.text().toLowerCase().replaceAll(" ", "")
  if (!deviceInfo || !deviceId || !deviceInfo?.modelURL){
    return
  } 
  def deviceChild = getChildDevice(deviceId)
  if (deviceChild){ 
    return
  }
  def deviceMetadata = getdeviceMetadata(deviceInfo)
  if (deviceMetadata == null) {
    log.warn "4. FAILED Creating device ${deviceInfo}"
    return
  }
  log.debug "4. Creating device ${deviceId}:${deviceMetadata}"
  try {
    def newDevice = addChildDevice(deviceMetadata.namespace, deviceMetadata.handler, deviceId, null, 
      [ 
        "label":  deviceInfo.friendlyName,
        "data": [
          "deviceid": "${deviceInfo.deviceId}",
          "devicenumber": "${deviceInfo.deviceIndex}"
        ]
      ]
    )
    def newDeviceJson = groovy.json.JsonOutput.toJson(newDevice)
    log.debug("4.1. Created device: ${newDeviceJson}")
    if (deviceMetadata.attribute != "") {
      sendEvent(newDevice, [name: deviceMetadata.attribute, value: deviceMetadata.attributeValue, displayed: true])
    }
    subscribeToDeviceEvents(newDevice, deviceInfo)
  } catch(e) {
    log.warn "4.1 addChildDevice failed: ${e.message}"
  }  
}

def registerHubIntoDevice(hubitat.device.HubResponse hubResponse) {
  log.debug "5. Registering Hub into device"
  def parsedResponse = parseLanMessage(hubResponse.description)
  String host = "${convertHexToIP(parsedResponse.ip)}:${convertHexToInt(parsedResponse.port)}"
  def api = getFullLocalApiServerUrl()
  def namespace = hubNamespace()
  if(!state?.accessToken) { 
    createAccessToken() 
  }
  def registrationPath = "/registerHub"
  def body = [ api: api, token: state.accessToken, namespace: namespace ]
  def json = groovy.json.JsonOutput.toJson(body)
  log.debug "  POST ${host}${registrationPath} BODY: ${json}"
  def length = json.size()
  sendHubCommand(new hubitat.device.HubAction([
    method: "POST",
    path: registrationPath,
    headers: [ HOST: host, "Content-Type": "application/json", "Content-Length": length],
    body : json
  ], host, [callback: confirmedHubRegistration]))
}

def confirmedHubRegistration(hubitat.device.HubResponse hubResponse){
  def parsedResponse = parseLanMessage(hubResponse.description)
  String host = "${convertHexToIP(parsedResponse.ip)}:${convertHexToInt(parsedResponse.port)}"
  log.debug "6. confirmed Hub Registration ${host}"
  state.isHubRegistered = true
}

// HTTP Request handlers (calls from device)
//**************************************************************
//path("/heartbeat") { action: [ POST: "handleDeviceHeartbeat"] }
def handleDeviceHeartbeat(){
  def deviceId = request.JSON?.content?.deviceId.toLowerCase()
  def eventName = request.JSON?.content?.eventName
  def eventValue = request.JSON?.content?.eventValue
  def deviceTypeId = request.JSON?.content?.deviceTypeId
  def deviceChild = getChildDevice(deviceId)
  if (!deviceChild) {
    logError("handleDeviceHeartbeat(InvalidDeviceId): ${deviceId}:${eventName}")
    httpError(501, "${deviceId} is not a valid device id")
  }
  state.handleDeviceEventTimeStamp = now()
  handleHeartbeat(deviceChild, deviceTypeId, eventName, eventValue)
  log.debug "handleDeviceHeartbeat: ${deviceId}:${eventName}:${eventValue}"
}

//path("/deviceEvent") { action: [ POST: "handleDeviceEvent"] }
def handleDeviceEvent(){
  // https://docs.smartthings.com/en/latest/smartapp-web-services-developers-guide/smartapp.html#response-handling
  //sendEvent(deviceId: deviceId, name: eventName, value: eventValue)
  def deviceId = request.JSON?.content?.deviceId.toLowerCase()
  def eventName = request.JSON?.content?.eventName.toLowerCase()
  def eventValue = request.JSON?.content?.eventValue
  def deviceTypeId = request.JSON?.content?.deviceTypeId

  def deviceChild = getChildDevice(deviceId)
  if (!deviceChild) {
    logError("handleDeviceEvent(InvalidDeviceId): ${deviceId}:${eventName}")
    httpError(501, "${deviceId} is not a valid device id")
  }
  state.handleDeviceEventTimeStamp = now()

  if (eventName == "Pushed".toLowerCase()){
    deviceChild.push(1)
  } else if (eventName == "Released".toLowerCase()) {
    deviceChild.release(1)
  } else if (eventName == "Closed".toLowerCase()) {
    deviceChild.close()
  } else if (eventName == "Opened".toLowerCase()) {
    deviceChild.open()
  } else if (eventName == "Active".toLowerCase()) {
    deviceChild.active()
  } else if (eventName == "Inactive".toLowerCase()) {
    deviceChild.inactive()
  } else if (eventName == "Illuminance".toLowerCase()) {
    deviceChild.illuminance(eventValue)
  } else if (eventName == "On".toLowerCase()) {
    deviceChild.on()
  } else if (eventName == "Off".toLowerCase()) {
    deviceChild.off()
  } else if (eventName == "Flow".toLowerCase()) {
    sendEvent(deviceChild, [name: "rate", value: eventValue])
  } else if (eventName == "Temperature".toLowerCase()) {
    sendEvent(deviceChild, [name: "temperature", value: eventValue, unit: "F"])
  } else if (eventName == "Humidity".toLowerCase()) {
    sendEvent(deviceChild, [name: "humidity", value: eventValue])
  } else {
    logError("handleDeviceEvent(InvalidEvent): ${deviceId}:${eventName}")
    httpError(501, "$eventName is not a valid event for the device")
    return
  }
  log.debug "handleDeviceEvent: ${deviceId}:${eventName}:${eventValue}"
}

def handleHeartbeat(deviceChild, deviceTypeId, eventName, eventValue) {
  switch (deviceTypeId) {
    case "push-button": 
      def currentValue = deviceChild.currentValue("button");
      if (currentValue != "pushed" && eventName == "Pushed") {
        deviceChild.push(1)
        log.debug "Button state: current='${currentValue}', new='${eventName}' "
      } else if (currentValue != "released" && eventName == "Released") {
        deviceChild.release(1)
        log.debug "Contact state: current='${currentValue}', new='${eventName}' "
      } else {
        log.debug "ERROR: Invalid Contact state: current='${currentValue}', new='${eventName}' "
      }
      return
    case "contact":
      def currentValue = deviceChild.currentValue("contact");
      if (currentValue != "open" && eventName == "Opened") {
        deviceChild.open()
        log.debug "Contact state: current='${currentValue}', new='${eventName}' "
      } else if (currentValue != "closed" && eventName == "Closed") {
        deviceChild.close()
        log.debug "Contact state: current='${currentValue}', new='${eventName}' "
      } else {
        log.debug "ERROR: Invalid Contact state: current='${currentValue}', new='${eventName}' "
      }
      return      
    case "motion-sensor":
      def currentValue = deviceChild.currentValue("motion");
      if (currentValue != "active" && eventName == "Active") {
        deviceChild.sendEvent(name: "motion", value: "active")
        log.debug "Motion state: current='${currentValue}', new='${eventName}' "
      } else if (currentValue != "inactive" && eventName == "Inactive") {
        deviceChild.sendEvent(name: "motion", value: "inactive")
        log.debug "Motion state: current='${currentValue}', new='${eventName}' "
      }
      return
    case "on-off-switch":
      def currentValue = deviceChild.currentValue("switch");
      if (currentValue != "on" && eventName == "On") {
        deviceChild.on()
        log.debug "Switch state: current='${currentValue}', new='${eventName}' "
      } else if (currentValue != "off" && eventName == "Off") {
        deviceChild.off()
        log.debug "Switch state: current='${currentValue}', new='${eventName}' "
      }
      return
    case "switch-relay":
      def currentValue = deviceChild.currentValue("switch");
      if (currentValue != "on" && eventName == "On") {
        deviceChild.on()
        log.debug "switch-relay state: current='${currentValue}', new='${eventName}' "
      } else if (currentValue != "off" && eventName == "Off") {
        deviceChild.off()
        log.debug "switch-relay state: current='${currentValue}', new='${eventName}' "
      }
      return
    case "flow-rate":
      sendEvent(deviceChild, [name: "rate", value: eventValue])
      log.debug "flow-rate state: current='${currentValue}', new='${eventValue}' "
      return
    case "temp-sensor":
      sendEvent(deviceChild, [name: "temperature", value: eventValue])
      log.debug "temp-sensor state: current='${currentValue}', new='${eventValue}' "
      return
    case "humidity-sensor":
      sendEvent(deviceChild, [name: "humidity", value: eventValue])
      log.debug "humidity-sensor state: current='${currentValue}', new='${eventValue}' "
      return
    
    default: 
      logError("No Heartbeat handler found for '${deviceTypeId}'")
  }

}

// Handle events from Hub
//**************************************************************
def buttonEvent(evt){    
  if (evt.value == "pushed") {
    genericEventHandler(evt, "0")
  } else {
    genericEventHandler(evt, "1")
  }
}

def contactEvent(evt){
  if (evt.value == "open") {
    genericEventHandler(evt, "1")
  } else if (evt.value == "closed") {
    genericEventHandler(evt, "0")
  } else {
    log.debug "Unhandled contactEvent: ${evt}"
  }  
}

def switchEvent(evt){
  if (evt.value == "off") {
    genericEventHandler(evt, "1")
  } else if (evt.value == "on") {
    genericEventHandler(evt, "0")
  } else {
    log.debug "Unhandled switchEvent: ${evt}"
  }
}

def genericEventHandler(evt, newState){
  if (state.handleDeviceEventTimeStamp && (now() - state.handleDeviceEventTimeStamp) < 100) {
    log.debug "event was generated from device: ${evt.descriptionText}"
    return;
  }
  log.debug "event: ${evt.descriptionText}"
  def deviceId = evt.device.data.deviceid.toLowerCase()
  def deviceIndex = evt.device.data.devicenumber
  def body = "deviceId=${deviceId}&deviceIndex=${deviceIndex}&state=${newState}"
  log.debug body
  if (!deviceId || !deviceIndex || !state.masterNodeIP) {
    discoverySearch()
    return
  }

  sendHubCommand(new hubitat.device.HubAction(
    [
      method:"POST",
      path: "/setDeviceState",
      body: body,
      headers: [ 
        HOST: state.masterNodeIP,
        "content-type": "application/x-www-form-urlencoded"
      ]
    ], state.masterNodeIP)
  )
}

// Constants & Helper functions
//**************************************************************
def subscribeToDeviceEvents(newDevice, deviceInfo) {
  switch (deviceInfo.deviceType) {
    case "urn:schemas-upnp-org:device:bothouse:push-button": 
      // https://docs.smartthings.com/en/latest/capabilities-reference.html#button
      subscribe(newDevice, "button", buttonEvent)
      log.debug "Subscribed to button events"
      return
    case "urn:schemas-upnp-org:device:bothouse:contact":
      // https://docs.smartthings.com/en/latest/capabilities-reference.html#contact-sensor
      subscribe(newDevice, "contact", contactEvent)
      log.debug "Subscribed to contact events"
      return      
    case "urn:schemas-upnp-org:device:bothouse:motion-sensor":
      // https://docs.hubitat.com/index.php?title=Driver_Capability_List#MotionSensor
      subscribe(newDevice, "motion", switchEvent)
      log.debug "Subscribed to motion events"
      return
    case "urn:schemas-upnp-org:device:bothouse:on-off-switch":
      // https://docs.smartthings.com/en/latest/capabilities-reference.html#switch
      subscribe(newDevice, "switch", switchEvent)
      log.debug "Subscribed to switch events"
      return
    case "urn:schemas-upnp-org:device:bothouse:switch-relay":
      // https://docs.smartthings.com/en/latest/capabilities-reference.html#relay-switch
      subscribe(newDevice, "switch", switchEvent)
      log.debug "Subscribed to switch events"
      return
    case "urn:schemas-upnp-org:device:bothouse:flow-rate":
      return
    case "urn:schemas-upnp-org:device:bothouse:temperatue":
      return
    case "urn:schemas-upnp-org:device:bothouse:humidity":
      return
    default: 
      logError("No Child Device Handler case for ${deviceInfo.deviceType}")
      return
  }
}

def getdeviceMetadata(deviceInfo) {
  switch (deviceInfo.deviceType) {
    case "urn:schemas-upnp-org:device:bothouse:push-button": 
      return [
        namespace: hubNamespace(),
        handler: "Virtual Button",
        attribute: "numberOfButtons",
        attributeValue: 1
      ]
    case "urn:schemas-upnp-org:device:bothouse:contact":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Contact Sensor",
        attribute: "",
        attributeValue: 0
      ]      
    case "urn:schemas-upnp-org:device:bothouse:motion-sensor":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Multi Attribute Sensor",
      ]
    case "urn:schemas-upnp-org:device:bothouse:on-off-switch":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Switch",
        attribute: "numberOfButtons",
        attributeValue: 1
      ]
    case "urn:schemas-upnp-org:device:bothouse:switch-relay":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Switch",
        attribute: "numberOfButtons",
        attributeValue: 1
      ]
    case "urn:schemas-upnp-org:device:bothouse:flow-rate":
      return [
        namespace: namespace(),
        handler: "Virtual Multi Attribute Sensor",
        attribute: "rate",
        attributeValue: 0
      ]    
    case "urn:schemas-upnp-org:device:bothouse:temp-sensor":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Temperature Sensor",
        attribute: "temperature",
        attributeValue: 0
      ]    
    case "urn:schemas-upnp-org:device:bothouse:humidity-sensor":
      return [
        namespace: hubNamespace(),
        handler: "Virtual Humidity Sensor",
        attribute: "humidity",
        attributeValue: 0
      ]    
    default: 
      logError("No Child Device Handler case for ${deviceInfo.deviceType}")
      return null
  }
}

def logError(message) {
  log.error message
  state.lastError = message
}

def logInfo(message) {
  log.info message
  state.info = message
}

def discoveryDeviceType() { return "urn:schemas-upnp-org:device:bothouse" }
def appName() { return "BotHouse" }
def hubNamespace() { return "hubitat" }
def namespace() { return "bot.local" }
def baseUrl() { return "bot.local" }
def getDeviceIpAndPort(device) {
  "${convertHexToIP(device.networkAddress)}:${convertHexToInt(device.deviceAddress)}"
}
private Integer convertHexToInt(hex) { Integer.parseInt(hex,16) }
private String convertHexToIP(hex) { 
  [convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".") 
}
