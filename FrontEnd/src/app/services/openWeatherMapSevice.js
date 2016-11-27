(function(){
    var app = angular.module('app');
    app.service('openWeatherMapSevice', function($http, $q){

        var apiKey = '5f51e2a10a9a4778a96bfee0ce8b87fd';

        function getGeoLocation(callback){
            if(navigator.geolocation){
                navigator.geolocation.getCurrentPosition(function(position){
                    if(callback){
                        callback(position);
                    }
                });
            }
        }

        this.requestWeather = function(){
            var defer = $q.defer();
            getGeoLocation(function(position){
                var lat = position.coords.latitude;
                var lon = position.coords.longitude;
                defer.resolve($http.get(
                    'http://api.openweathermap.org/data/2.5/weather?appid=' + 
                        apiKey + '&lat=' + lat + '&lon=' + lon).then(function(result){
                            return result.data;
                        }));
            });
            return defer.promise;
        };

        this.extractWeather = function(data){
            return {
                Icon: "wi wi-owm-" + data.weather[0].id,
				Celcius:  (data.main.temp / 10).toFixed(2),
				Humidity: data.main.humidity,
				Pressure : data.main.pressure / 10,
                Name: data.name
            };
		}
    });
})();