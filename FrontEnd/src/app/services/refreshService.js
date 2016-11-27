(function(){
    var app = angular.module('app');
    app.service('refreshService', function(){

        var timers = [];
        var id = 0;

        this.start = function(delay, refresh){
            var timer = new Tock({
                interval: delay,
                callback: refresh,
            });
            timer.start();
            id++;
            timers.push({
                ID: id,
                timer: timer
            });
            return id;
        };

        this.stop = function(id){
            var timer = _.find(timers, function(timer){
                return timer.ID === id;
            });
            if(timer){
                timer.stop();
                timer.reset();
            }
        };
    });
})();