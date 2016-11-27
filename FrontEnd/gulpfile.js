var gulp = require('gulp');
var webserver = require('gulp-webserver');
var inject = require('gulp-inject');
var less = require('gulp-less');
var path = require('path');
watch = require('gulp-watch');
 
gulp.task('webserver', function() {
  gulp.src('src')
    .pipe(webserver({
      livereload: true,
	    open: true,
      fallback: "index.html",
      https: true,
    }));
});

gulp.task('watch-less', function () {
    return watch('./src/less/**/*.less', function () {
        return gulp.src('./src/less/**/*.less')
                    .pipe(less())
                    .pipe(gulp.dest('./src/app/css'));
    });
});

gulp.task('watch-css', function () {
    return watch('./src/app/css/*.css', function () {
        var target = gulp.src('./src/index.html');
        var sources = gulp.src('./src/**/*.css', {
          read: false
        });
      
        return target.pipe(inject(sources, {
          relative: true
        })).pipe(gulp.dest('./src'));
    });
});

gulp.task('watch-js', function () {
    return watch('./src/app/**/*.js', function () {
        var target = gulp.src('./src/index.html');
        var sources = gulp.src('./src/**/*.js', {
          read: false
        });
      
        return target.pipe(inject(sources, {
          relative: true
        })).pipe(gulp.dest('./src'));
    });
});

gulp.task('default', ['webserver', 'watch-less', 'watch-css', 'watch-js']);