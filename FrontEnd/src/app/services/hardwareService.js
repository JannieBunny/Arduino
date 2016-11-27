(function(){
    var app = angular.module('app');
    app.service('hardwareService', 
        function(deviceConnectionsService, refreshService, $timeout, 
                    $rootScope){

            var self = this;
            var devicesOnline = [];
            var sensorsOnline = [];

            self.updateGPIO = updateGPIO;
            
            self.getGPIO = function(){
                requestGPIOData();
                devicesOnline = [];
                $timeout(function(){
                    $rootScope.$broadcast('devices-online', _.map(devicesOnline, function(device){
                        return device.ID;
                    }));
                }, 3000);
            }

            self.getSensors = function(){
                requestSensorsData();
                sensorsOnline = [];
                $timeout(function(){
                    $rootScope.$broadcast('sensors-online', _.map(sensorsOnline, function(sensor){
                        return sensor.ID;
                    }));
                }, 3000);
            };
            
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

            $rootScope.$on('message-received', function(event, args){
			    $timeout(function(){
                    //Get connected devices
                    _.each(args, function(device){
                        var isAdded = _.find(devicesOnline, function(deviceOnline){
                            return deviceOnline.ID === device.ID;
                        });
                        if(!isAdded){
                            devicesOnline.push(device);
                        }
                        //GPIO Update
                        if(device.UpdateType === 0){
                            $rootScope.$broadcast('gpio-updated', {
                                ID: device.ID,
                                GPIO: device.GPIO
                            });
                        }
                        if(device.UpdateType > 0){
                            $rootScope.$broadcast('sensors-updated', device);
                        }
                    });
			    });
		    });
    });
})();