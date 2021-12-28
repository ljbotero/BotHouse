// window.services = {
//   getConfig: function (successCallback) {
//     $.ajax({
//       type: 'GET', url: '/config', dataType: 'json',
//       success: successCallback,
//       error: function (xhr, type, error) { alert(xhr.response); }
//     })
//   },
//   getPoll: function (successCallback) {
//     $.ajax({ type: 'GET', url: '/poll', success: successCallback });
//   },
//   getLogs: function (successCallback, errorCallback) {
//     $.ajax({
//       type: 'GET', url: '/logs', dataType: 'json',
//       data: { newerThanTimestamp: newerThanTimestamp },
//       success: successCallback,
//       error: errorCallback
//     });
//   }

// };

window.services = {
  getConfig: function (successCallback) {
    successCallback(window.mockReplies.getConfig);
  },
  getPoll: function (successCallback) {
    successCallback(window.mockReplies.getPoll);
  },
  getLogs: function (successCallback, errorCallback) {
    successCallback(window.mockReplies.getLogs);
  }
};

window.mockReplies = {
  getConfig: {
    "action": "sharedInfo",
    "content": {
      "info": {
        "time": 1810932,
        "connectedToWifiRouter": true
      },
      "storage": {
        "version": 6,
        "state": 128,
        "wifiName": "WIFI-NETWORK",
        "wifiPassword": "**********",
        "hubApi": "192.168.2.229/apps/api/417",
        "hubToken": "**********",
        "hubNamespace": "hubitat",
        "deviceName": "",
        "deviceTypeId": "on-off-switch"
      }
    }
  },
  getPoll: {
    "action": "meshInfo",
    "content": [
      {
        "deviceId": "d6ef", "deviceName": "Upstairs: Mariana Bath Relay", "wifiSSID": "BOTEROS-NET", "wifiRSSI": -49, "isMaster": 1, "IPAddress": "192.168.2.16", "apSSID": "BotHouse3f7_1", "apLevel": 1, "freeHeap": 20560, "lastUpdate": 3988280, "systemTime": 3988280, "devices": [{ "deviceIndex": 0, "deviceTypeId": "switch-relay", "deviceState": "Off" }], "accessPoints": [{ "SSID": "BotHouse3f7_6", "isRecognized": true, "isOpen": false, "RSSI": -70 }, { "SSID": "BotHouse3f7_7", "isRecognized": true, "isOpen": false, "RSSI": -68 }, { "SSID": "BotHouse3f7_8", "isRecognized": true, "isOpen": false, "RSSI": -85 }]
      }, {
        "deviceId": "42e7",
        "deviceName": "42e7",
        "wifiSSID": "BOTEROS-NET",
        "wifiRSSI": -66, "isMaster": 0,
        "IPAddress": "192.168.2.80",
        "apSSID": "BotHouse3f7_8",
        "apLevel": 8,
        "freeHeap": 22560,
        "lastUpdate": 3949934,
        "systemTime": 3949934,
        "devices": [
          { "deviceIndex": 3, "deviceTypeId": "contact", "deviceState": "Opened" },
          { "deviceIndex": 2, "deviceTypeId": "contact", "deviceState": "Opened" },
          { "deviceIndex": 1, "deviceTypeId": "contact", "deviceState": "Opened" },
          { "deviceIndex": 0, "deviceTypeId": "contact", "deviceState": "Opened" }],
        "accessPoints": [
          { "SSID": "BotHouse3f7_7", "isRecognized": true, "isOpen": false, "RSSI": -69 },
          { "SSID": "BotHouse3f7_1", "isRecognized": true, "isOpen": false, "RSSI": -80 },
          { "SSID": "BotHouse3f7_6", "isRecognized": true, "isOpen": false, "RSSI": -83 },
          { "SSID": "YingGongFu_0276BC", "isRecognized": false, "isOpen": false, "RSSI": -91 }]
      }, {
        "deviceId": "7901", "deviceName": "Upstairs: Mariana Bath Switch", "wifiSSID": "BotHouse3f7_1", "wifiRSSI": -46, "isMaster": 0, "IPAddress": "192.168.4.100", "apSSID": "", "apLevel": 0, "freeHeap": 23552, "lastUpdate": 3714490, "systemTime": 3714490, "devices": [{ "deviceIndex": 0, "deviceTypeId": "on-off-switch", "deviceState": "Off" }], "accessPoints": [{ "SSID": "BotHouse3f7_8", "isRecognized": true, "isOpen": false, "RSSI": -74 }, { "SSID": "BotHouse3f7_7", "isRecognized": true, "isOpen": false, "RSSI": -54 }, { "SSID": "BotHouse3f7_1", "isRecognized": true, "isOpen": false, "RSSI": -37 }, { "SSID": "BotHouse3f7_6", "isRecognized": true, "isOpen": false, "RSSI": -61 }]
      }, {
        "deviceId": "d51d", "deviceName": "Downstairs: Bathroom Light", "wifiSSID": "BOTEROS-NET", "wifiRSSI": -48, "isMaster": 0, "IPAddress": "192.168.2.240", "apSSID": "BotHouse3f7_7", "apLevel": 7, "freeHeap": 23336, "lastUpdate": 3949662, "systemTime": 3949662, "devices": [{ "deviceIndex": 0, "deviceTypeId": "switch-relay", "deviceState": "Off" }], "accessPoints": [{ "SSID": "BotHouse3f7_1", "isRecognized": true, "isOpen": false, "RSSI": -70 }, { "SSID": "BotHouse3f7_6", "isRecognized": true, "isOpen": false, "RSSI": -72 }, { "SSID": "BotHouse3f7_8", "isRecognized": true, "isOpen": false, "RSSI": -75 }]
      }, {
        "deviceId": "9686", "deviceName": "Upstairs: Luz Baño Niños", "wifiSSID": "BOTEROS-NET", "wifiRSSI": -65, "isMaster": 0, "IPAddress": "192.168.2.252", "apSSID": "", "apLevel": 6, "freeHeap": 19392, "lastUpdate": 3981609, "systemTime": 3981609, "devices": [{ "deviceIndex": 0, "deviceTypeId": "switch-relay", "deviceState": "Off" }], "accessPoints": [{ "SSID": "BotHouse3f7_1", "isRecognized": true, "isOpen": false, "RSSI": -70 }, { "SSID": "BotHouse3f7_7", "isRecognized": true, "isOpen": false, "RSSI": -73 }, { "SSID": "BotHouse3f7_8", "isRecognized": true, "isOpen": false, "RSSI": -88 }]
      }, {
        "deviceId": "25a4", "deviceName": "Downstairs: Office Light (new)", "wifiSSID": "BotHouse3f7_7", "wifiRSSI": -71, "isMaster": 0, "IPAddress": "192.168.4.100", "apSSID": "", "apLevel": 0, "freeHeap": 23368, "lastUpdate": 3950203, "systemTime": 3950203, "devices": [{ "deviceIndex": 0, "deviceTypeId": "switch-relay", "deviceState": "Off" }], "accessPoints": [{ "SSID": "BotHouse3f7_7", "isRecognized": true, "isOpen": false, "RSSI": -64 }, { "SSID": "BotHouse3f7_1", "isRecognized": true, "isOpen": false, "RSSI": -67 }, { "SSID": "BotHouse3f7_6", "isRecognized": true, "isOpen": false, "RSSI": -85 }, { "SSID": "BotHouse3f7_8", "isRecognized": true, "isOpen": false, "RSSI": -68 }, { "SSID": "YingGongFu_0276BC", "isRecognized": false, "isOpen": false, "RSSI": -86 }]
      }
    ]
  },
  getLogs: [
    {
      "timestamp": 4060632,
      "deviceName": "3ab1d0",
      "message": "  (MessageProcessor) processMessage:deviceInfo"
    },
    {
      "timestamp": 4062195,
      "deviceName": "3ae6be",
      "message": "                              (MessageBroadcast) 192.168.2.255"
    },
    {
      "timestamp": 4062163,
      "deviceName": "3ae6be",
      "message": "                              (MessageProcessor) processMessage:pollDevices                              (MessageBroadcast) broadcastMessage - localIP: "
    },
    {
      "timestamp": 4060152,
      "deviceName": "3ab1d0",
      "message": "192.168.2.255"
    },
    {
      "timestamp": 4060150,
      "deviceName": "3ab1d0",
      "message": "(WebServer) handlePollGet  (MessageBroadcast) broadcastMessage - localIP: "
    }
  ]
};