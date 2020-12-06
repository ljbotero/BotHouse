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
        "deviceTypeId": "on-off-switch",
        "amazonEmail": "test@test.com"
      }
    }
  },
  getPoll: {
    "action": "meshInfo",
    "content": [
      {
        "deviceId": "d929",
        "deviceName": "Oficina",
        "wifiSSID": "WIFI-NETWORK",
        "wifiRSSI": -61,
        "isMaster": 1,
        "IPAddress": "192.168.2.220",
        "apSSID": "BotLocale8e_1",
        "apLevel": 1,
        "freeHeap": 22656,
        "lastUpdate": 1100996948,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId":
              "switch-relay",
            "deviceState": "On"
          }
        ],
        "accessPoints":
          [
            { "SSID": "BotLocale8e_5", "isRecognized": true, "isOpen": false, "RSSI": -57 },
            { "SSID": "BotLocale8e_4", "isRecognized": true, "isOpen": false, "RSSI": -77 },
            { "SSID": "BotLocale8e_2", "isRecognized": true, "isOpen": false, "RSSI": 0 }
          ]
      },
      {
        "deviceId": "8d40",
        "deviceName": "Botones Alexa",
        "wifiSSID": "WIFI-NETWORK",
        "wifiRSSI": -37, "isMaster": 0,
        "IPAddress": "192.168.2.245",
        "apSSID": "BotLocale8e_3",
        "apLevel": 3,
        "freeHeap": 28472,
        "lastUpdate": 1100749874,
        "devices": [
          {
            "deviceIndex": 3,
            "deviceTypeId": "contact",
            "deviceState": "Closed"
          },
          {
            "deviceIndex": 2,
            "deviceTypeId": "contact",
            "deviceState": "Opened"
          },
          {
            "deviceIndex": 1,
            "deviceTypeId": "contact",
            "deviceState": "Closed"
          }
        ], "accessPoints": []
      },
      {
        "deviceId": "7901",
        "deviceName": "Upstairs-Switch",
        "wifiSSID": "WIFI-NETWORK",
        "wifiRSSI": -52,
        "isMaster": 0,
        "IPAddress": "192.168.2.198",
        "apSSID": "BotLocale8e_2",
        "apLevel": 2,
        "freeHeap": 17544,
        "lastUpdate": 1100831066,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "on-off-switch",
            "deviceState": "On"
          }],
        "accessPoints": [
          { "SSID": "BotLocale8e_3", "isRecognized": true, "isOpen": false, "RSSI": -63 },
          { "SSID": "BotLocale8e_1", "isRecognized": true, "isOpen": false, "RSSI": -78 }
        ]
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