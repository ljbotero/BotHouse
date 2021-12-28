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
};