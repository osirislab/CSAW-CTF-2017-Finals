var deleteObj = function(name) {
    $.ajax({
        url: '/api/delete',
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({
            name: name,
        }),
        success: function(result) {
            $.ajax({
                url: 'http://'+ip+':6002/action',
                method: 'POST',
                contentType: 'application/json',
                data: JSON.stringify({'action':result.action}),
                success: function() {
                    updateList();
                },
                error: function() {
                    $('#objectList').prepend('<div class="alert alert-danger">Could not delete object...</div>');
                }
            });
        },
        error: function() {
            $('#objectList').prepend('<div class="alert alert-danger">Could not sign delete request...</div>');
        }
    });
}

var viewObj = function(name) {
    $.ajax({
        url: '/api/get',
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({
            name: name,
        }),
        success: function(result) {
            $.ajax({
                url: 'http://'+ip+':6002/action',
                method: 'POST',
                contentType: 'application/json',
                data: JSON.stringify({'action':result.action}),
                success: function(result) {
                    var f = new JSONFormatter.default(JSON.parse(result));
                    $('#preview').empty();
                    $('#preview').append(f.render());
                },
                error: function() {
                    $('#preview').html('<div class="alert alert-danger">Unable to get object...</div>');
                }
            });
        },
        error: function() {
            $('#preview').html('<div class="alert alert-danger">Unable to sign get for object...</div>');
        }
    });
}

var share = function(name) {
    $.ajax({
        url: '/api/share',
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({
            name: name,
        }),
        success: function(result) {
            var m = $('<div>',{'class':'modal fade',tabindex:"-1",role:"dialog"});
            m.html('<div class="modal-dialog modal-sm" role="document"><div class="modal-content">'+
                    '<div class="modal-header">'+
                    '<button type="button" class="close" data-dismiss="modal" aria-label="Close">'+
                    '<span aria-hidden="true">&times;</span></button>'+
                    '<h4 class="modal-title">Shareable Link</h4>'+
                    '</div>'+
                    '<div class="modal-body"><div class="form-group"><input class="form-control link">'+
                    '</div></div></div></div>');
            m.modal();
            m.find('.link')[0].value = 'http://'+ip+':6002/share?sig='+encodeURIComponent(result.sig)+'&type=json';
            m.on('shown.bs.modal', function() {
                m.find('.link').focus();
                m.find('.link').select();
            });
        },
        error: function() {
            $('#preview').html('<div class="alert alert-danger">Unable to sign get for object...</div>');
        }
    });
}


var updateList = function() {
    $.ajax({
        url: 'http://'+ip+':6002/action',
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({'action': objectListRequest}),
        success: function(result) {
            $('#objectList').empty();
            if (!result.names || result.names.length === 0) {
                $('#objectList').html('<div class="text-muted text-center">No Objects Found</div>');
                return;
            }

            for (var i in result.names) {
                var n = result.names[i];
                var c = $('<div>',{'class':'col-md-4'});
                c.html('<div class="panel panel-default"><div class="panel-body">'+
                        '<strong/><span class="pull-right">'+
                        '<button class="delete btn btn-danger">'+
                        '<i class="fa fa-trash-o" title="delete"></i></button>&nbsp;'+
                        '<button class="view btn btn-success">'+
                        '<i class="fa fa-eye" title="view"></i></button>&nbsp;'+
                        '<button class="share btn btn-info">'+
                        '<i class="fa fa-share-alt" title="share"></i></button>'+
                        '</span></div></div>');
                c.find('strong').text(n);
                c.find('.delete').click((name => (function() {
                    if( confirm("Are you sure you want to delete "+name+"?")) {
                        deleteObj(name);
                    }
                }))(n));

                c.find('.view').click((name => (function() {
                    viewObj(name);
                }))(n));

                c.find('.share').click((name => (function() {
                    share(name);
                }))(n));

                $('#objectList').append(c);
            }
        },
        error: function() {
            $('#objectList').html('<div class="alert alert-danger">Could not load objects...</div>');
        }
    });
}
updateList();

$('#object').keyup(function() {
    try {
        var j = JSON.parse($('#object')[0].value);
        var f = new JSONFormatter.default(j);
        $('#preview').empty();
        $('#preview').append(f.render());
    } catch (e) {
        $('#preview').html('<div class="alert alert-warning">Unable to decode JSON object...</div>');
    }
});

$('#create').click(function() {
    var name = $('#name')[0].value;
    if (name === '') {
        $('#alert').html('<div class="alert alert-danger">A name is required</div>');
        $('#name').focus();
        return;
    }

    try {
        var j = JSON.parse($('#object')[0].value);
    } catch (e) {
        $('#alert').html('<div class="alert alert-danger">Unable to decode JSON object...</div>');
        $('#object').focus();
        return;
    }

    $.ajax({
        url: '/api/serialize',
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({
            name: name,
            object: j
        }),
        success: function(result) {
            $.ajax({
                url: 'http://'+ip+':6002/action',
                method: 'POST',
                contentType: 'application/json',
                data: JSON.stringify({'action':result.action}),
                success: function(res) {
                    if (!res.stored)
                        return $('#alert').html('<div class="alert alert-danger">Could not store object...</div>');
                    $('#alert').html('<div class="alert alert-success">Successfully stored object!</div>');
                    $('#name')[0].value = '';
                    $('#object')[0].value = '';
                    updateList();
                },
                error: function() {
                    $('#alert').html('<div class="alert alert-danger">Could not store object...</div>');
                }
            });
        },
        error: function() {
            $('#alert').html('<div class="alert alert-danger">Could not sign object</div>');
        }
    });
});


