(function(){
    var app = angular.module('app');
    app.factory('menuFactory', function(openWeatherMapSevice){

        var loadedTiles = [];
        var tiles = {
            getOpenWeatherTile : getOpenWeatherTile,
            updateOpenWeatherTile : updateOpenWeatherTile
        };

        function getOpenWeatherTile(){
            if(getTile(-1)){
                return;
            }
            var openWeatherMaps = {
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

        function updateOpenWeatherTile(weatherData){
            var tile = getTile(-1);
            if(tile && weatherData){
                tile.icon = weatherData.Icon;
                tile.title = "Online Weather for " + weatherData.Name;
                _.each(tile.data, function(tileData){
                    var propertyData = weatherData[tileData.title];
                    if(propertyData){
                        tileData.value = propertyData;
                    }
                });
            }
        }

        function getTile(id){
            return _.find(loadedTiles, function(tile){
                return tile.Type === id;
            })
        }

        return tiles;
    });
})();