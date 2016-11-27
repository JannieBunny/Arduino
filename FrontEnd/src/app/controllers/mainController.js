(function(){
    var app = angular.module('app');
    app.controller('mainController', 
		function($scope, deviceConnectionsService, $timeout, tileFactory, refreshService, 
					hardwareService, softwareService){

		$scope.tiles = [];
		deviceConnectionsService.init();

		//Refresh every 15mins
		refreshService.start(10000, function(){
			//Hardware functions is websockets based, so we don't wait
			hardwareService.getGPIO();
			hardwareService.getSensors();

			softwareService.get().then(function(data){
				var tile = tileFactory.generateWeatherTile(-1);
				if(tile){
					$scope.tiles.push(tile);
				}
				tileFactory.updateWeatherTile(data, -1, true);
			});
		});

		$scope.menu = [{
			title: "Connected Sensors",
			value: 1,
			icon: "mif-broadcast"
		},
		{
			title: "Connected Devices",
			value: 1,
			icon: "mif-usb"
		}];

		$scope.$on('devices-online', function(event, args){
			_.remove($scope.tiles, function(tile){
				if(tile){
					return args.indexOf(tile.ID) === -1 && tile.Type > 0;
				}
				return false;
			});
			_.each(args, function(arg){
				var tileExist = _.find($scope.tiles, function(tile){
					return tile.ID === arg && tile.Type > 0;
				});
				if(!tileExist){
					Array.prototype.push.apply($scope.tiles, tileFactory.generateDeviceGPIOTiles(args, function(pin, value, id){
						hardwareService.updateGPIO(id, pin, value ? 1 : 0);
					}));
				}
			});
        });

		$scope.$on('gpio-updated', function(event, args){
			tileFactory.updateDeviceGPIOTiles(args.ID, args.GPIO);
		});

		$scope.$on('sensors-updated', function(event, args){
			var tile = tileFactory.generateWeatherTile(args.ID);
			if(tile){
				$scope.tiles.push(tile);
			}
			args.Pressure = args.Pressure / 1000;
			tileFactory.updateWeatherTile(args, args.ID);
		});
    });
})();