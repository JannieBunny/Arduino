(function(){
    var app = angular.module('app');
    app.service('deviceConnectionsService', function($websocket, $rootScope){

        var socketUrl = "ws://192.168.8.107:8090";
        var socket;

        this.init = function(){
            socket = $websocket(socketUrl);
            socket.onMessage(function(message) {
                $rootScope.$broadcast('message-received', JSON.parse(message.data));
            });
        };

        this.sendData = function(data){
            socket.send(JSON.stringify(data));   
        };
    });
})();