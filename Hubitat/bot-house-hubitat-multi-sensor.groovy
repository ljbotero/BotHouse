/**
 *  Virtual Multi Attribute Sensor
 *
 */
metadata {
	definition (
    name: "Virtual Multi Attribute Sensor", 
    namespace: "${namespace()}", 
    author: "Jaime Botero") 
    {
      capability "LiquidFlowRate"
      capability "MotionSensor"
      capability "IlluminanceMeasurement"
      capability "Sensor"

      command "reset"
      command "showers", [[name:"dailyShowerCount",type:"NUMBER", description:"Daily showers count"]]
     command "illuminance", [[name:"lux",type:"NUMBER", description:"Illuminance in Lux"]]
     command "active"
     command "inactive"

      attribute "lastUpdated", "String"
      attribute "rate", "NUMBER"
      attribute "dailyShowerCount", "NUMBER"
      attribute "illuminance", "NUMBER"      
      attribute "motion", "String"
      attribute "LastMeasurement", "DATE"
    }
}

def showers(dailyShowerCount) {
    log.debug "dailyShowerCount: (${dailyShowerCount})"
    sendEvent(name: "dailyShowerCount", value: dailyShowerCount)
}

def illuminance(lux) {
    log.debug "illuminance: (${lux})"
    sendEvent(name: "illuminance", value: lux)
}

def active() {
    log.debug "motion: active"
    sendEvent(name: "motion", value: "active")
}

def inactive() {
    log.debug "motion: inactive"
    sendEvent(name: "motion", value: "inactive")
}

def reset() {
    state.clear()
    sendEvent(name: "rate", value: 0)
}

def parse(String description) {
  log.debug "parse(${description}) called"
  def parts = description.split(" ")
  def name  = parts.length>0?parts[0].trim():null
  def value = parts.length>1?parts[1].trim():null
  if (name && value) {
      // Update device
      sendEvent(name: name, value: value)
      // Update lastUpdated date and time
      def nowDay = new Date().format("MMM dd", location.timeZone)
      def nowTime = new Date().format("h:mm a", location.timeZone)
      sendEvent(name: "lastUpdated", value: nowDay + " at " + nowTime, displayed: false)
  }
  else {
    log.warn "Missing either name or value.  Cannot parse!"
  }
}

def installed() {
    refresh()
}

def updated() {
    refresh()
}

def refresh() {
    parent.updateStatus()
}

def appName() { return "BotHouse" }
def hubNamespace() { return "hubitat" }
def namespace() { return "bot.local" }
def baseUrl() { return "bot.local" }
