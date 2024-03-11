
$(document).ready(function(){
    //connect to the socket server.
    var socket = io.connect('http://' + document.domain + ':' + location.port + '/update');

    //receive details from server
    socket.on('newdata', function(msg) {
        console.log("Received data");
        $('#data').html(msg.data);
    });

});