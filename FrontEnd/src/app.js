var app = angular.module('app', ['angular-websocket', 'ui.router']);

app.config(function($stateProvider, $urlRouterProvider) {

    $urlRouterProvider.otherwise("/");
    $stateProvider.state('main', {
        url: "/",
        templateUrl: "app/views/main.html"
    });
});