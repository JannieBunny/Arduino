(function(){
    var app = angular.module('app');
    app.service('softwareService', 
        function(openWeatherMapSevice){
            this.get = function(){
                return openWeatherMapSevice.requestWeather().then(function(data){
                    return openWeatherMapSevice.extractWeather(data);
                });
            };
    });
})();