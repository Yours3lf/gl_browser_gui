//cpp to js calls

//this gets called when the page is loaded
//do js initialization (to make sure cpp-js interaction works)
function bindings_complete()
{
  init_js();
}

//example cpp to js function call
function cpp_to_js(str)
{
  alert(str);
  
  document.title = "hi there|";
  document.body.style.cursor = 'wait';
  
  //TODO doesn't work
  //window.resizeTo(1600, 900); //doesn't work
  
  resize_window("1600 900"); //workaround
  window.open("http://www.w3schools.com");
}

//example file chooser callback function
function filechooser_callback(str)
{
  alert(str);
}