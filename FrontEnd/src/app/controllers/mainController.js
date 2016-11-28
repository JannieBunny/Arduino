(function(){
    var app = angular.module('app');
    app.controller('mainController', 
		function($scope, deviceConnectionsService, $timeout, tileFactory, refreshService, 
					hardwareService, softwareService){

		$scope.tiles = [];
		deviceConnectionsService.init();

		refreshService.start(2000, function(){
			//Hardware functions is websockets based, so we don't wait
			hardwareService.getGPIO();
			hardwareService.getSensors();
			softwareService.get().then(function(data){
				var tile = tileFactory.generateWeatherTile(-1);
				if(tile && !_.find($scope.tiles, function(tileExist){
					return tile.ID === tileExist.ID && tileExist.Type < 0;
				})){
					$scope.tiles.push(tile);
				}
				tileFactory.updateWeatherTile(data, -1, true);
			});
		});

		$scope.menu = [{
			ID: 1,
			title: "Connected Sensors",
			value: 0,
			icon: "mif-broadcast"
		}];

		$scope.$on('devices-online', function(event, args){
			_.remove($scope.tiles, function(tile){
				if(tile){
					return _.map(args, 'ID').indexOf(tile.ID) === -1 && tile.ID > 0;
				}
				return false;
			});
			_.each(args, function(arg){
				//Find GPIO Tiles
				var tileExist = _.find($scope.tiles, function(tile){
					return tile.ID === arg.ID && tile.Type === 0;
				});
				if(!tileExist){
					Array.prototype.push.apply($scope.tiles, 
						tileFactory.generateDeviceGPIOTiles(_.map(args, 'ID'), function(pin, value, id){
							hardwareService.updateGPIO(id, pin, value ? 1 : 0);
					}, arg.FriendlyName));
				}
			});

			var menuItem = _.find($scope.menu, function(menuItem){
				return menuItem.ID === 1;
			});

			menuItem.value = _.uniq(_.map(_.filter($scope.tiles, function(tile){
				return tile.ID > 0 && tile.Type === 0;
			}), 'ID')).length;
        });

		$scope.$on('gpio-updated', function(event, args){
			tileFactory.updateDeviceGPIOTiles(args.ID, args.GPIO);
		});

		$scope.$on('sensors-updated', function(event, args){
			if(args.UpdateType === -1){
				var tile = tileFactory.generateWeatherTile(args.ID);
				if(tile && !_.find($scope.tiles, function(tileExist){
					return tile.ID === tileExist.ID && tileExist.Type < 0;
				})){
					$scope.tiles.push(tile);
				}
				args.Pressure = args.Pressure / 1000;
				args.Name = args.FriendlyName; 
				tileFactory.updateWeatherTile(args, args.ID);
			}
		});
    });
})();