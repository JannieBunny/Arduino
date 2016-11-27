(function(){
    var app = angular.module('app');
    app.controller('mainController', 
		function($scope, deviceConnectionsService, $timeout, menuFactory, refreshService, 
					hardwareService, softwareService){

		var data1 = [];
		var data2 = [];
		for(var a = 1; a <= 4; a++){
			data1.push({
				title: "Control: " + a.toString(),
				value: false,
				execute: invokeGPIOUpdate,
				pinValue: a
			});
			data2.push({
				title: "Control: " + (4 + a).toString(),
				value: false,
				execute: invokeGPIOUpdate,
				pinValue: (4 + a)
			});
		}

		function invokeGPIOUpdate(pin, value, id){
			hardwareService.updateGPIO(id, pin, value ? 1 : 0);
		}

		$scope.tiles = [{
				ID: 1,
				Type: 1,
				color: "bg-darkPink",
				icon: "mif-cloud",
				title: "Sensor Weather",
					data:[{
						title: "Celcius",
						value: 0,
						metric: "C"	
					},
					{
						title: "Humidity",
						value: 0,
						metric: "%"
					},
					{
						title: "Pressure",
						value: 0,
						metric: "kpa"
					}]
			},
			{
				ID: 1,
				Type: 0,
				color: "bg-amber",
				icon: "mif-upload2",
				title: "GPIO",
				data: data1,
				canEdit: true
			},
			{
				ID: 1,
				Type: 0,
				color: "bg-pink",
				icon: "mif-download2",
				title: "GPIO",
				data: data2,
				canEdit: true
			}
		];

		$scope.tiles.push(menuFactory.getOpenWeatherTile())
		deviceConnectionsService.init();

		//Refresh every 15mins
		refreshService.start(60000 * 15, function(){
			//Hardware functions is websockets based, so we don't wait
			hardwareService.get();

			softwareService.get().then(function(data){
				menuFactory.updateOpenWeatherTile(data);
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
		},
		{
			title: "Configure Sensors",
			icon: "fa fa-gear"
		},
		{
			title: "Configure Devices",
			icon: "fa fa-gear"
		}];

		$scope.$on('message-received', function(event, args){
			$timeout(function(){
				//Sensors
				_.each(args, function(arg){
					var tiles = _.filter($scope.tiles, function(tile){
						return tile.ID == arg.ID && arg.UpdateType === tile.Type;
					});
					_.each(tiles, function(tile){
						for (var property in arg) {
							if (arg.hasOwnProperty(property)){
								if(arg[property].constructor !== Array){
									var dataProperty = _.find(tile.data, function(data){
										return data.title === property;
									});
									if(dataProperty){
										dataProperty.value = arg[property];
									}
								}
							}
						}
					});
				});
				//GPIO
				_.each(args, function(arg){
					var tiles = _.filter($scope.tiles, function(tile){
						return tile.ID == arg.ID && arg.UpdateType === tile.Type;
					});
					_.each(tiles, function(tile){
						var gpio = arg.GPIO;
						_.each(gpio, function(pin){
							var dataProperty = _.find(tile.data, function(data){
								return data.title.indexOf(pin.Pin.toString()) !== -1;
							});
							if(dataProperty){
								dataProperty.value = pin.Value === 0 ? false : true;
							}
						});
					});
				});
			});
		});
    });
})();