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
  let deviceId = $(e.currentTarget).attr("data-deviceId");
  if (!deviceId) { return; }
  $.ajax({ type: 'POST', url: '/restart', data: { deviceId: deviceId } });
  alert("Restart signal has been sent to device")
};

let removeDevice = function (e) {
  let deviceId = $(e.currentTarget).attr("data-deviceId");
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
  $('#wifiName').val(data.content.storage.wifiName);
  $('#wifiPassword').val(data.content.storage.wifiPassword);
  $('#hubNamespace').val(data.content.storage.hubNamespace);
  $("#deviceTypeId").val(data.content.storage.deviceTypeId);

  $('#originalWifiName').val(data.content.storage.wifiName);
  $('#originalWifiPassword').val(data.content.storage.wifiPassword);
  $('#connectedToWifiRouter').val(data.content.info.connectedToWifiRouter ? '1' : '0');
};

// Logs Fetching
let startLogs = function () {
  try {
    var connection = new WebSocket('ws://' + location.hostname + ':90');
    connection.onopen = function () {
      connection.send('Connect ' + new Date());
    };
    connection.onerror = function (error) {
      console.log('WebSocket Error ', error);
    };
    connection.onmessage = function (e) {
      let msg = e.data;
      if (msg.includes("[ERROR]")) {
        console.error(msg);
      } else if (msg.includes("[WARNING]")) {
        console.warn(msg);
      } else {
        console.log(e.data);
      }
    };
    connection.onclose = function () {
      console.log('WebSocket connection closed');
      //setTimeout(startLogs, 5000);
    };
  } catch {
    console.log("Failed connecting to logs WebSocket");
  }

}

let refreshConfigDevicesHandler;

let refreshConfigDevices = function () {
  window.services.getPoll(function (devices) {
    if (devices && isNonEmptyArray(devices.content)) {
      window.devices = devices;
      showDevices(devices);
    }
    if (refreshConfigDevicesHandler) {
      clearTimeout(refreshConfigDevicesHandler);
    }
    refreshConfigDevicesHandler = setTimeout(refreshConfigDevices, 1000 * 30);
  });
}

// Setup Navigation
let screenRefreshed = function () {

  if (location.hash === "#config-general") {
    window.services.getConfig(processConfig);
  } else if (!location.hash || location.hash === "#config-devices") {
    if (window.devices) { showDevices(window.devices); }
    refreshConfigDevices();
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
  data.content.forEach((node) => {
    let seconds = Math.floor((node.systemTime / 1000) % 60);
    let minutes = Math.floor((node.systemTime / (1000 * 60)) % 60);
    let hours = Math.floor((node.systemTime / (1000 * 60 * 60)));
    node.systemTimeMinutes = ("00" + hours).slice(-2) + ":" + ("00" + minutes).slice(-2);
  });
  let rendered = Mustache.render(template, data);
  $('#devices-content').html(rendered);
  data.content.forEach((node) => {
    $("#deviceTypeId_" + node.deviceId).text(node.deviceTypeId);
    node.systemTimeMinutes = node.systemTime / (60 * 60);
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
  startLogs();
  refreshScreen();

  $("#discover-devices").click(function () {
    location.hash = "#config-discover";
  });

  $("#refresh-devices").click(function () {
    refreshConfigDevices();
  });

  $("#ping").click(function () {
    $.ajax({ type: 'GET', url: '/ping' });
    $("#ping").prop('disabled', true);
    setTimeout(function () { $("#ping").prop('disabled', false); }, 3000);
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