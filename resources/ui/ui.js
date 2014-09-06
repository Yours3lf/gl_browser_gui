//startup

//don't rely on this to initialize stuff
$(function()
{
	$('button').button();
	$(document).tooltip();
}
);

//do any js initialization here!
function init_js()
{  
  alert("js initialized");
  console.log("hi there");
}