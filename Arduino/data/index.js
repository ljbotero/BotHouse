// Utils
let millisBaseline;

let getFormattedTime = function (d) {
  let hr = d.getHours();
  let min = d.getMinutes();
  let secs = d.getSeconds();
  let ampm = "am";
  if (hr > 12) {
    hr -= 12;
    ampm = "pm";
  }
  return hr + ":" + (min < 10 ? '0' : '') + min + ":" + (secs < 10 ? '0' : '') + secs + ampm;
}

let restartDevice = function (e) {
  $(e.target).prop('disabled', true);
  setTimeout(function () { $(e.target).prop('disabled', false); }, 15000);
  let deviceId = $(e.target).attr("data-deviceId");
  if (!deviceId) { return; }
  $.ajax({ type: 'POST', url: '/restart', data: { deviceId: deviceId } });
  newerThanTimestamp = 0;
  alert("Restart signal has been sent to device")
};

let removeDevice = function (e) {
  let deviceId = $(e.target).attr("data-deviceId");
  if (!deviceId) { return; }
  if (prompt("Please type the word \"REMOVE\" to confirm device removal").toUpperCase() === 'REMOVE') {
    $.ajax({ type: 'POST', url: '/removeDevice', data: { deviceId: deviceId } });
    location.hash = "#config-devices";
    setTimeout(function () {
      alert("Restart signal has been sent to remove device. Please refresh this screen "
        + "in a few minutes to validate device has been removed.");
    }, 1000);
  }
};

let isNonEmptyArray = function (arr) {
  return arr && Array.isArray(arr) && arr.length > 0;
}

// Load configuration            
let processConfig = function (data) {
  if (!data || !data.content || !data.content.storage) { return; }
  $('#meshName').val(data.content.storage.meshName);
  $('#meshPassword').val(data.content.storage.meshPassword);
  $('#wifiName').val(data.content.storage.wifiName);
  $('#wifiPassword').val(data.content.storage.wifiPassword);
  $('#hubNamespace').val(data.content.storage.hubNamespace);
  $("#deviceTypeId").val(data.content.storage.deviceTypeId);

  $('#originalMeshName').val(data.content.storage.meshName);
  $('#originalMeshPassword').val(data.content.storage.meshPassword);
  $('#originalWifiName').val(data.content.storage.wifiName);
  $('#originalWifiPassword').val(data.content.storage.wifiPassword);
  $('#connectedToWifiRouter').val(data.content.info.connectedToWifiRouter ? '1' : '0');
  let amazonIntegrationText = "Amazon Echo";
  if (data.content.storage.amazonEmail) {
    amazonIntegrationText += " (" + data.content.storage.amazonEmail + ")";
  }
  $('#config-amazon-integration').text(amazonIntegrationText);
};

// Logs Fetching
let newerThanTimestamp = 0;
let fetchLogsTimeout;
let showLogs = function () {
  if (fetchLogsTimeout) {
    clearTimeout(fetchLogsTimeout);
  }
  fetchLogsTimeout = setTimeout(fetchLogs, 5000);
};

let clearLogs = function () {
  newerThanTimestamp = 0;
  let $logMessages = $("#log-messages");
  $logMessages.val("");
};

let fetchLogs = function () {
  window.services.getLogs(processFetechedLogs,
    function (xhr, type, error) {
      clearLogs();
      screenRefreshed();
    }
  );
};

let processFetechedLogs = function (data, status, xhr) {
  if (data && Array.isArray(data) && data.length > 0) {
    newerThanTimestamp = data[0].timestamp;
    data.reverse().forEach(function (item, index) {
      let $logMessages = $("#log-messages");
      millisBaseline = millisBaseline && newerThanTimestamp <= item.timestamp ?
        millisBaseline : Date.now() - item.timestamp;
      let timestamp = new Date(millisBaseline + item.timestamp);
      let formattedTime = getFormattedTime(timestamp);
      $logMessages.val($logMessages.val() + "\n" + formattedTime + "/"
        + item.deviceName + "-> " + item.message);
      $logMessages.scrollTop($logMessages[0].scrollHeight);
    })
  }
  screenRefreshed();
};

let registerDevicesIntoCloud = function (tokens, data) {
  $("#config-amazon-progress").text("(Please wait) Registering device to Alexa skill");
  let getMessage = () => {
    let payload = {
      userId: data.user_id,
      tokens: tokens,
      endpoints: []
    };

    window.devices.content.forEach((node) => {
      node.devices.forEach((device) => {
        let endpoint = {
          endpointId: node.deviceId + ".Button." + device.deviceIndex,
          manufacturerName: "BotLocal",
          friendlyName: node.deviceName + " " + device.deviceIndex,
          description: "BotLocal button" + device.deviceIndex,
          displayCategories: ["CONTACT_SENSOR"],
          capabilities: []
        };

        if (device.deviceTypeId === 'contact') {
          endpoint.capabilities.push({
            type: "AlexaInterface",
            interface: "Alexa.ToggleController",
            instance: "Button." + device.deviceIndex,
            version: "3",
            properties: {
              supported: [
                {
                  name: "toggleState"
                }
              ],
              proactivelyReported: true,
              retrievable: true,
              nonControllable: true
            },
            capabilityResources: {
              friendlyNames: [
                {
                  "@type": "text",
                  value: {
                    locale: "en-US",
                    text: "push button"
                  }
                }
              ]
            }
          });
        }
        if (endpoint.capabilities.length > 0) {
          endpoint.capabilities.push({
            type: "AlexaInterface",
            interface: "Alexa",
            version: "3"
          });
          payload.endpoints.push(endpoint);
        }
      });
    });

    let message = {
      directive: {
        header: {
          namespace: "Localbot",
          name: "Register"
        },
        payload: payload
      }
    };
    return message;
  };

  const skillLink = "https://skills-store.amazon.com/deeplink/tvt/756b493e49ae2e152988262525c7054c8101089a54d1e73397d44ce24d249c82c7d4027610230f8a91c50bebf99b95038355b9faf691c685590886f1551bc68e90363d21855b15f0c3ef17744c5dbe5410843f814fe5d107d71aca96a7551be2035515d463fd2845705887e5dc8c72";

  let success = () => {
    $("#config-amazon-progress").html(
      "<div><a href='" + skillLink + "'>Click here</a> "
      + "to install<br/>The BotLocal Alexa Skill</div>")
  };

  if (!window.devices) {
    window.services.getPoll(function (devices) {
      if (devices && isNonEmptyArray(devices.content)) {
        window.devices = devices;
        window.services.registerDevicesIntoCloud(tokens, getMessage(), success, processHttpError);
      }
    });
  } else {
    window.services.registerDevicesIntoCloud(tokens, getMessage(), success, processHttpError);
  }


};

let getAmazonProfile = function (tokens) {
  $("#configAmazonForm").hide();
  $("#config-amazon-progress").text("(Please wait) Fetching Amazon profile");
  window.services.getAmazonProfile(tokens, function (data) {
    window.services.persistAmazonProfile(tokens, data, () => { }, processHttpError);
    registerDevicesIntoCloud(tokens, data);
  }, processHttpError);
};

let getAmazonTokens = function (authData) {
  $("#config-amazon-progress").text("");
  window.services.getAmazonTokens(authData, getAmazonProfile,
    function () {
      setTimeout(function () {
        if (location.hash === "#config-amazon") {
          getAmazonTokens(authData);
        }
      }, 3000);
    }
  );
};

// Setup Navigation
let screenRefreshed = function () {

  if (location.hash === "#config-general") {
    window.services.getConfig(processConfig);
  } else if (location.hash === "#config-amazon") {
    $("#configAmazonForm").show();
    $("#config-amazon-progress").text("");
    var userCode = $('#user_code').val();
    if (!userCode) {
      window.services.getAmzDeviceAuth(function (authData) {
        $('#config-amazon-integration').show();
        $('#config-verification-uri').text(authData.verification_uri);
        $('#config-verification-uri').click(() => {
          window.open(authData.verification_uri, "_blank");
        });
        $('#config-amazon-user-code').text(authData.user_code);
        $('#user_code').val(authData.user_code);
        $('#device_code').val(authData.device_code);
        getAmazonTokens(authData);
      });
    }
  } else if (location.hash === "#config-logs") {
    showLogs();
  } else if (!location.hash || location.hash === "#config-devices") {
    if (window.devices) { showDevices(window.devices); }
    window.services.getPoll(function (devices) {
      if (devices && isNonEmptyArray(devices.content)) {
        window.devices = devices;
        showDevices(devices);
      }
    });
  } else if (location.hash === "#config-device" && location.search.startsWith("?deviceId=")) {
    let deviceId = location.search.substr("?deviceId=".length);
    if (window.devices) {
      showDevice(window.devices, deviceId);
    } else {
      window.services.getPoll(function (devices) {
        if (devices && isNonEmptyArray(devices.content)) {
          window.devices = devices;
          showDevice(devices, deviceId);
        }
      });
    }
  } else if (location.hash === "#config-discover") {
    if (window.devices) { showAccessPoints(window.devices); }
    window.services.getPoll(function (devices) {
      if (devices && isNonEmptyArray(devices.content)) {
        window.devices = devices;
        showAccessPoints(devices);
      }
    });
  }
};

let refreshScreen = function () {
  $("article > section").hide();
  setTimeout(function () {
    if (location.hash.length > 1) {
      $(location.hash).show();
    } else {
      $("#config-devices").show();
    }
    screenRefreshed();
  }, 100);
};

// Setup events
let complementDevicesData = function (data) {
  if (!data) { return; }
  data.lastUpdateFormatted = function () {
    millisBaseline = millisBaseline ? millisBaseline : Date.now() - this.lastUpdate;
    let timestamp = new Date(millisBaseline + this.lastUpdate);
    return getFormattedTime(timestamp);
  };
}

let showDevice = function (data, deviceId) {
  if (!data || !deviceId) { return; }
  let deviceData = data.content.find(d => d.deviceId === deviceId);
  if (!deviceData) { return; }
  complementDevicesData(deviceData);
  let template = $('#device-template').html();
  Mustache.parse(template);
  let rendered = Mustache.render(template, deviceData);
  $('#device-content').html(rendered);
  for (let device of deviceData.devices) {
    $("#deviceTypeId-" + device.deviceIndex).val(device.deviceTypeId);
  }
  $(document).on("click", "#device-action-restart", function (e) {
    e.preventDefault();
    restartDevice(e);
  });
  $(document).on("click", "#device-action-remove", function (e) {
    e.preventDefault();
    removeDevice(e);
  });
  $(document).on("click", '#devices-action-save', function (e) {
    e.preventDefault();
    let form = ".deviceform[data-deviceId='" + deviceId + "']";
    let data = {
      deviceId: deviceId,
      deviceName: $(form + " #deviceName").val(),
    };
    let deviceIndex = 0;
    let deviceTypeIdElementName = form + " #deviceTypeId-" + deviceIndex;
    while ($(deviceTypeIdElementName).val()) {
      data["deviceTypeId" + deviceIndex] = $(deviceTypeIdElementName).val()
      deviceIndex++;
      deviceTypeIdElementName = form + " #deviceTypeId-" + deviceIndex;
    }
    $.ajax({
      type: 'POST',
      url: '/updateDevice',
      data: data,
      success: function (data) {
        window.devices = undefined;
        screenRefreshed();
      },
      error: processHttpError
    });
  });

};

let toggleDeviceState = function (caller, deviceId, deviceIndex, deviceState) {
  $.ajax({
    type: 'POST',
    url: '/toggleDeviceState',
    data: { deviceId: deviceId, deviceIndex: deviceIndex },
    success: function (data) {
      //screenRefreshed();
      // if (data) {
      //   $(caller).val(data);
      // }
    },
    error: processHttpError
  }
  );
};

let showDevices = function (data) {
  if (!data) { return; }
  complementDevicesData(data);
  let template = $('#devices-template').html();
  Mustache.parse(template);
  let rendered = Mustache.render(template, data);
  $('#devices-content').html(rendered);
  data.content.forEach((node) => {
    $("#deviceTypeId_" + node.deviceId).text(node.deviceTypeId);
  });
};

let addDevice = function (caller, SSID) {
  $.ajax({
    type: 'POST', url: '/addDevice', data: { SSID: SSID }
  });
  setTimeout(function () {
    alert('Trying to add device...\nPlease refresh this page in a few minutes.');
  }, 1000);
};

let showAccessPoints = function (data) {
  if (!data) { return; }
  // Gather all access points dedup and sort
  let accessPoints = [];
  data.content
    .filter(device => device.accessPoints)
    .forEach(device => accessPoints = accessPoints.concat(device.accessPoints));
  // Dedup and Remove already added devices
  const validNodeName = /(\S+)_-?(\d+)/;
  let dedupped = [];
  accessPoints
    .filter(ap => ap.isOpen && validNodeName.test(ap.SSID))
    .forEach(ap => {
      if (!dedupped.find(ap2 => ap.SSID === ap2.SSID)
        && !data.content.find(device => ap.SSID === device.apSSID || ap.SSID === device.wifiSSID)) {
        dedupped.push(ap);
      }
    });

  // Render template
  let template = $('#discovery-template').html();
  Mustache.parse(template);
  let rendered = Mustache.render(template, dedupped);
  $('#discovery-content').html(rendered);

};

let processHttpError = function (xhr, type, error) {
  alert(xhr.response);
};

let init = function () {
  refreshScreen();

  $("#discover-devices").click(function () {
    location.hash = "#config-discover";
  });

  $("#ping").click(function () {
    $.ajax({ type: 'GET', url: '/ping' });
    $("#ping").prop('disabled', true);
    setTimeout(function () { $("#ping").prop('disabled', false); }, 3000);
  });

  $("#clear-logs").click(function () {
    newerThanTimestamp = 0;
    let $logMessages = $("#log-messages");
    $logMessages.val("");
    fetchLogs();
    $("#clear-logs").prop('disabled', true);
    setTimeout(function () { $("#clear-logs").prop('disabled', false); }, 1000);
  });

  $('#configform').submit(function (e) {
    e.preventDefault();

    $.ajax({
      type: 'POST',
      url: '/update',
      data: $('#configform').serializeArray(),
      success: function (data) { alert(data); },
      error: processHttpError
    });

    let connectedToWifiRouter = $('#connectedToWifiRouter').val();
    if (connectedToWifiRouter == '1' && (
      $('#originalWifiName').val() != $('#wifiName').val() ||
      $('#originalWifiPassword').val() != $('#wifiPassword').val())) {
      alert("Since you are already connected to WiFi and you are changing WiFi settings, "
        + "you might lose access to this page for a moment "
        + "until validation finishes.\n"
        + "Please reload this page in about a minute.");
    }

  });
};