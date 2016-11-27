(function(){
    var app = angular.module('app');
    app.service('refreshService', function(){

        var timer;

        this.start = function(delay, refresh){
            if(timer){
                timer.interval = delay;
            }
            else{
                timer = new Tock({
                    interval: delay,
                    callback: refresh,
                });
            }
            timer.start();
        };

        this.stop = function(){
            if(timer){
                timer.stop();
                timer.reset();
            }
        };
    });
})();