$.ajax({
    url: 'http://'+ip+':6002/action',
    method: 'POST',
    contentType: 'application/json',
    data: JSON.stringify({'action': objectListRequest}),
    success: function(result) {
        $('#objectCount').text(result.names.length.toString());
    },
});

$('#viewAlerts').click(function(e) {
    $('#alerts').dropdown('toggle');
    e.preventDefault();
    return false;
});
