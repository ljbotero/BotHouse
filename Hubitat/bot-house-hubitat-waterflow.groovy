/**
 *  Virtual Water Flow Sensor
 *
 */
metadata {
	definition (
    name: "Virtual Water Flow Sensor", 
    namespace: "${namespace()}", 
    author: "Jaime Botero") 
    {
      capability "LiquidFlowRate"
      capability "Sensor"

      command "reset"

      attribute "lastUpdated", "String"
      attribute "rate", "NUMBER"
      attribute "LastMeasurement", "DATE"
    }
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
