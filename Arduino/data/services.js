<<<<<<< HEAD
window.services = {
  getConfig: function (successCallback) {
    $.ajax({
      type: 'GET', url: '/config', dataType: 'json',
      success: successCallback,
      error: function (xhr, type, error) { alert(xhr.response); }
    })
  },
  getPoll: function (successCallback) {
    $.ajax({ type: 'GET', url: '/poll', success: successCallback });
  },
  getLogs: function (successCallback, errorCallback) {
    $.ajax({
      type: 'GET', url: '/logs', dataType: 'json',
      data: { newerThanTimestamp: newerThanTimestamp },
      success: successCallback,
      error: errorCallback
    });
  }
=======
window.services = {
  registerDevicesIntoCloud: function (tokens, message, successCallback, errorCallback) {
    const url = "https://l32ezbt5b8.execute-api.us-east-1.amazonaws.com/default";
    const apiKey = "4rHOU0GUJv3rzDUEcMAUv5dq0fSweJsg3MGlpEfI";
    $.ajax({
      type: 'POST',
      url: url,
      dataType: 'json',
      contentType: 'application/json',
      processData: false, 
      headers: {
        'x-api-key': apiKey
      },
      data: JSON.stringify(message),
      success: successCallback,
      error: errorCallback
    });
  },
  persistAmazonProfile: function (tokens, data, successCallback, errorCallback) {
    $.ajax({
      type: 'POST',
      url: '/saveAmazonProfile',
      data: {
        refresh_token: tokens.refresh_token,
        user_id: data.user_id,
        email: data.email
      },
      success: successCallback,
      error: errorCallback
    })
  },
  getAmazonProfile: function (tokens, successCallback, errorCallback) {
    $.ajax({
      type: 'GET',
      url: 'https://api.amazon.com/user/profile',
      headers: {
        Authorization: tokens.token_type + " " + tokens.access_token
      },
      success: successCallback,
      error: errorCallback
    })
  },
  getAmazonTokens: function (authData, successCallback, errorCallback) {
    $.ajax({
      type: 'POST',
      url: 'https://api.amazon.com/auth/o2/token',
      data: { grant_type: 'device_code', user_code: authData.user_code, device_code: authData.device_code },
      success: successCallback,
      error: errorCallback
    });
  },
  getAmzDeviceAuth: function (successCallback) {
    $.ajax({
      type: 'POST',
      url: 'https://api.amazon.com/auth/o2/create/codepair',
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      data: {
        "response_type": "device_code",
        "client_id": "amzn1.application-oa2-client.e7a0a88ade5c41f09c47fdba2e9452b6",
        "scope": "profile"
      },
      success: successCallback,
      error: function (xhr, type, error) { alert(xhr.response); }
    });
  },
  getConfig: function (successCallback) {
    $.ajax({
      type: 'GET', url: '/config', dataType: 'json',
      success: successCallback,
      error: function (xhr, type, error) { alert(xhr.response); }
    })
  },
  getPoll: function (successCallback) {
    $.ajax({ type: 'GET', url: '/poll', success: successCallback });
  },
  getLogs: function (successCallback, errorCallback) {
    $.ajax({
      type: 'GET', url: '/logs', dataType: 'json',
      data: { newerThanTimestamp: newerThanTimestamp },
      success: successCallback,
      error: errorCallback
    });
  }
>>>>>>> d396803 (First commit)
};