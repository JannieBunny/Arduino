(function(){
    var app = angular.module('app');
    app.factory('tileFactory', function(openWeatherMapSevice){

        var loadedTiles = [];
        var tiles = {
            generateWeatherTile : generateWeatherTile,
            updateWeatherTile : updateWeatherTile,
            generateDeviceGPIOTiles: generateDeviceGPIOTiles,
            updateDeviceGPIOTiles: updateDeviceGPIOTiles
        };

        function generateDeviceGPIOTiles(ids, callBack){
            var gpioDevices = [];
            _.each(ids, function(id){
                var data1 = [];
                var data2 = [];
                for(var a = 1; a <= 4; a++){
                    //Hardware ids start at 1, not zero
                    data1.push({
                        title: "Control: " + a.toString(),
                        value: false,
                        execute: callBack,
                        pinValue: a
                    });
                    data2.push({
                        title: "Control: " + (a + 4).toString(),
                        value: false,
                        execute: callBack,
                        pinValue: a + 4
                    });
                }
                var tile1 = {
                    ID: id,
                    Type: 1,
                    color: "bg-amber",
                    icon: "fa fa-lightbulb-o",
                    title: "GPIO",
                    data: data1,
                    canEdit: true
                };
                var tile2 = {
                    ID: id,
                    Type: 1,
                    color: "bg-amber",
                    icon: "fa fa-lightbulb-o",
                    title: "GPIO",
                    data: data2,
                    canEdit: true
                };
                gpioDevices.push(tile1);
                gpioDevices.push(tile2);
            });
            Array.prototype.push.apply(loadedTiles, gpioDevices);
            return gpioDevices;
        }

        function updateDeviceGPIOTiles(id, gpio){
            var tiles = getGPIOTiles(id);
            if(!tiles){
                return;
            }
            _.each(tiles, function(tile){
                _.each(tile.data, function(data){
                    gpioToAppply = _.find(gpio, function(port){
                        return port.Pin === data.pinValue;
                    });
                    if(gpioToAppply){
                        data.value = gpioToAppply.Value === 1 ? true : false;
                    }
                });
            });
        }

        function generateWeatherTile(id){
            if(getTile(id, -1)){
                return;
            }
            var openWeatherMaps = {
                ID: id,
				Type: -1,
				color: "bg-orange",
                icon: "",
				title: "",
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
			};
            loadedTiles.push(openWeatherMaps);
            return openWeatherMaps;
        }

        function updateWeatherTile(weatherData, id, isInternet){
            var tile = getTile(id, -1);
            if(tile && weatherData){
                if(!weatherData.Icon){
                    weatherData.Icon = "fa fa-cloud";
                }
                if(!weatherData.Name){
                    weatherData.Name = "local location"
                }
                tile.icon = weatherData.Icon;
                tile.title = (isInternet ? "Online " : "") + "Weather for " + weatherData.Name;
                _.each(tile.data, function(tileData){
                    var propertyData = weatherData[tileData.title];
                    if(propertyData){
                        tileData.value = propertyData;
                    }
                });
            }
        }

        function getTile(id, type){
            return _.find(loadedTiles, function(tile){
                return tile.Type === type && tile.ID === id;
            });
        }

        function getGPIOTiles(id){
            return _.filter(loadedTiles, function(tile){
                return tile.Type === id && tile.Type === 0;
            });
        }

        return tiles;
    });
})();