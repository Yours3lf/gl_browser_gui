//js to cpp calls

//we need a funcarray, so that we can call these functions later by name
//this is a bug workaround...
var funcarray = {};

function open_window(str){}
funcarray['open_window'] = function(str){ open_window(str); };

function resize_window(str){}
funcarray['resize_window'] = function(str){ resize_window(str); };

function choose_file(str){}
funcarray['choose_file'] = function(str){ choose_file(str); };

function js_to_cpp(){}
funcarray['js_to_cpp'] = function(){ js_to_cpp(); };