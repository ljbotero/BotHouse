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
        "buildNumber": "0",
        "deviceId": "4C7F68",
        "deviceName": "4C7F68_MotionSensor_Dining",
        "macAddress": "EC:FA:BC:4C:7F:68",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -65,
        "isMaster": 0,
        "IPAddress": "192.168.2.146",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 28720,
        "lastUpdate": 968497,
        "systemTime": 2741391,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "motion-sensor",
            "deviceState": "Inactive",
            "deviceValue": 0
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "D8D3C9",
        "deviceName": "D8D3C9_MotionSensor_Garage",
        "macAddress": "AC:0B:FB:D8:D3:C9",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -39,
        "isMaster": 0,
        "IPAddress": "192.168.2.115",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 28896,
        "lastUpdate": 1067844,
        "systemTime": 3375007,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "motion-sensor",
            "deviceState": "Inactive",
            "deviceValue": 0
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "130A46",
        "deviceName": "Water Heater Monitor Mariana",
        "macAddress": "2C:F4:32:13:0A:46",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -61,
        "isMaster": 0,
        "IPAddress": "192.168.2.127",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 28088,
        "lastUpdate": 1067874,
        "systemTime": 317187,
        "devices": [
          {
            "deviceIndex": 1,
            "deviceTypeId": "push-button",
            "deviceState": "Released",
            "deviceValue": -65403
          },
          {
            "deviceIndex": 0,
            "deviceTypeId": "color-control",
            "deviceState": "",
            "deviceValue": 1852793647
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "DEB44E",
        "deviceName": "DEB44E_Flow_Sensor",
        "macAddress": "50:02:91:DE:B4:4E",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -53,
        "isMaster": 0,
        "IPAddress": "192.168.2.165",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 30504,
        "lastUpdate": 1067523,
        "systemTime": 172140104,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "flow-rate",
            "deviceState": "NoFlow",
            "deviceValue": 0
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "C01D35",
        "deviceName": "C01D35_Mariana_Bath_Relay",
        "macAddress": "A4:CF:12:C0:1D:35",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -45,
        "isMaster": 0,
        "IPAddress": "192.168.2.88",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 29408,
        "lastUpdate": 1060445,
        "systemTime": 172093281,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "switch-relay",
            "deviceState": "Off",
            "deviceValue": 1
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "567901",
        "deviceName": "567901_Mariana Bath Switch",
        "macAddress": "CC:50:E3:56:79:01",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -24,
        "isMaster": 0,
        "IPAddress": "192.168.2.145",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 30192,
        "lastUpdate": 1067552,
        "systemTime": 319532,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "on-off-switch",
            "deviceState": "Off",
            "deviceValue": 0
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "0",
        "deviceId": "4C562D",
        "deviceName": "4C562D_MotionSensor_Patio",
        "macAddress": "EC:FA:BC:4C:56:2D",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -64,
        "isMaster": 0,
        "IPAddress": "192.168.2.93",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 28568,
        "lastUpdate": 1060503,
        "systemTime": 282882,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "motion-sensor",
            "deviceState": "Illuminance",
            "deviceValue": 100
          }
        ],
        "accessPoints": []
      },
      {
        "buildNumber": "202301140",
        "deviceId": "130C6C",
        "deviceName": "130C6C",
        "macAddress": "2C:F4:32:13:0C:6C",
        "wifiSSID": "BOTEROS-IOT",
        "wifiRSSI": -67,
        "isMaster": 0,
        "IPAddress": "192.168.2.99",
        "apSSID": "",
        "apLevel": 0,
        "freeHeap": 28296,
        "lastUpdate": 1073539,
        "systemTime": 1073520,
        "devices": [
          {
            "deviceIndex": 0,
            "deviceTypeId": "motion-sensor",
            "deviceState": "Inactive",
            "deviceValue": 0
          }
        ],
        "accessPoints": []
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