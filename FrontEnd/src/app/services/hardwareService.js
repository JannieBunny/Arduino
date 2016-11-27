(function(){
    var app = angular.module('app');
    app.service('hardwareService', 
        function(deviceConnectionsService){
            this.get = function(){
                requestSensorsData();
                requestGPIOData();
            };

            this.updateGPIO = updateGPIO;
            
            function requestSensorsData(){
                deviceConnectionsService.sendData({
                    Type: 'sensors'
                });
            }

            function requestGPIOData(){
                deviceConnectionsService.sendData({
                    Type: 'gpio/get'
                });
            }

            function updateGPIO(id, pin, value){
                deviceConnectionsService.sendData({
                    Type: 'gpio/update',
                    ID: id,
                    GPIO:[{
                        Pin: pin,
                        Value: value
                    }]
                });
            }
    });
})();