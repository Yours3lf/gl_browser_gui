#include <mymath/mymath.h>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <streambuf>
#include <list>
#include <vector>
#include <map>

#include "browser.h"

#include <sstream>
#include <string>
#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace mymath;
using namespace std;

#define INFOLOG_SIZE 4096
#define GET_INFOLOG_SIZE INFOLOG_SIZE - 1
#define STRINGIFY(s) #s

sf::Window the_window;
sf::Event the_event;

bool run = true;

//function prototypes
template< class t >
void handle_events( const t& f );
void compile_shader( const char* text, const GLuint& program, const GLenum& type, const std::string& additional_str );
void link_shader( const GLuint& shader_program );
GLuint create_quad( const vec3& ll, const vec3& lr, const vec3& ul, const vec3& ur );
void load_shader( GLuint& program, const GLenum& type, const string& filename, const std::string& additional_str = "" );
void get_opengl_error( const bool& ignore = false );

//NOTE important bits here come

//cpp to js functions that can be called from here
namespace js
{
  //this function gets called when a page is loaded
  //ie. you'd want to do any js initializing AFTER this point
  void bindings_complete( const browser_instance& w )
  {
    //override window.open function, so that creating a new berkelium window actually works
    //the berkelium api is broken... :D
    //this means that opening a window calls our function (see below)
    browser::get().execute_javascript( w,
    L"\
              window.open = function (open) \
                           { \
                                              return function (url, name, features) \
                                                                       { \
                                                                                                        open_window(url); \
                                                                                                                                                 return open.call ?  open.call(window, url, name, features):open( url, name, features); \
                                                                                                                                                                                                }; \
                                                                                                                                                                                                                                                   }(window.open);" );

    //for manually loaded webpages (loaded from the cpp code)
    //we add an onclick function call to the <input type="file" />
    //tags, so that file selection, opening etc. works
    //see below
    browser::get().execute_javascript( w,
    L"\
              var list = document.getElementsByTagName('input'); \
                           for( var i = 0; i < list.length; ++i ) \
                                            { \
                                                                   var att = list[i].getAttribute('type'); \
                                                                                                if(att && att == 'file') \
                                                                                                                                   { \
                                                                                                                                                                              list[i].onclick = function(){ choose_file('Open file of'); }; \
                                                                                                                                                                                                                               } \
                                                                                                                                                                                                                                                                                    } \
                                                                                                                                                                                                                                                                                                                                             " );

    std::wstringstream ws;
    ws << "bindings_complete();";

    browser::get().execute_javascript( w, ws.str() );
  }

  //example cpp to js function call
  void cpp_to_js( const browser_instance& w, const std::wstring& str )
  {
    std::wstringstream ws;
    ws << "cpp_to_js('" << str << "');";

    browser::get().execute_javascript( w, ws.str() );
  }
}

//NOTE broken, don't use
/*void browser::onResizeRequested( Berkelium::Window* win,
                        int x,
                        int y,
                        int newWidth,
                        int newHeight )
                        {
                        std::cout << "resize requested" << std::endl;

                        frm.resize( x, y, newWidth, newHeight );
                        }*/

//NOTE you can do whatever with the page title, I decided to print in to the top of the window
void browser::onTitleChanged( Berkelium::Window* win,
                              Berkelium::WideString title )
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
  std::wstring str( title.mData, title.mLength );
  the_window.setTitle( conv.to_bytes( str ) );
}

int main( int argc, char** argv )
{
  /*
   * Process program arguments
   */

  map<string, string> args;

  for( int c = 1; c < argc; ++c )
  {
    args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
    ++c;
  }

  cout << "Arguments: " << endl;
  for_each( args.begin(), args.end(), []( pair<string, string> p )
  {
    cout << p.first << " " << p.second << endl;
  } );

  uvec2 screen( 0 );
  bool fullscreen = false;
  bool silent = false;
  string title = "OpenGL Browser Based GUI";

  stringstream ss;
  ss.str( args["--screenx"] );
  ss >> screen.x;
  ss.clear();
  ss.str( args["--screeny"] );
  ss >> screen.y;
  ss.clear();

  if( screen.x == 0 )
  {
    screen.x = 1280;
  }

  if( screen.y == 0 )
  {
    screen.y = 720;
  }

  try
  {
    args.at( "--fullscreen" );
    fullscreen = true;
  }
  catch( ... )
  {
  }

  try
  {
    args.at( "--help" );
    cout << title << ", written by Marton Tamas." << endl <<
         "Usage: --silent      //don't display FPS info in the terminal" << endl <<
         "       --screenx num //set screen width (default:1280)" << endl <<
         "       --screeny num //set screen height (default:720)" << endl <<
         "       --fullscreen  //set fullscreen, windowed by default" << endl <<
         "       --help        //display this information" << endl;
    return 0;
  }
  catch( ... )
  {
  }

  try
  {
    args.at( "--silent" );
    silent = true;
  }
  catch( ... )
  {
  }

  /*
   * Initialize the OpenGL context
   */

  the_window.create( sf::VideoMode( screen.x > 0 ? screen.x : 1280, screen.y > 0 ? screen.y : 720, 32 ), title, fullscreen ? sf::Style::Fullscreen : sf::Style::Default );

  if( !the_window.isOpen() )
  {
    cerr << "Couldn't initialize SFML." << endl;
    the_window.close();
    return -1;
  }

  the_window.setPosition( sf::Vector2i( 0, 0 ) );

  GLenum glew_error = glewInit();

  glGetError(); //ignore glew errors

  cout << "Vendor: " << glGetString( GL_VENDOR ) << endl;
  cout << "Renderer: " << glGetString( GL_RENDERER ) << endl;
  cout << "OpenGL version: " << glGetString( GL_VERSION ) << endl;
  cout << "GLSL version: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << endl;

  if( glew_error != GLEW_OK )
  {
    cerr << "Error initializing GLEW: " << glewGetErrorString( glew_error ) << endl;
    the_window.close();
    return -1;
  }

  if( !GLEW_VERSION_4_3 )
  {
    cerr << "Error: " << STRINGIFY( GLEW_VERSION_4_3 ) << " is required" << endl;
    the_window.close();
    exit( 0 );
  }

  glEnable( GL_DEBUG_OUTPUT );

  the_window.setVerticalSyncEnabled( true );

  //set opengl settings
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ); //sky color
  glClearDepth( 1.0f );

  glViewport( 0, 0, screen.x, screen.y );

  get_opengl_error();

  /*
   * Set up the scene
   */

  GLuint ss_quad = create_quad( vec3( -1, -1, 0 ), vec3( 1, -1, 0 ), vec3( -1, 1, 0 ), vec3( 1, 1, 0 ) );

  /*
   * Set up the browser
   */

  //init with the path where the berkelium resources can be found
  //this only works with berkelium >11
  //for berkelium 8 (on linux) these need to be in the same folder as the exe
  browser::get().init( L"../resources/berkelium/win32" );

  //this is a browser window
  //you can have multiple, and render them whereever you'd like to
  //like, display a youtube video
  //run a webgl game on one of the walls etc.
  //you get a texture, and you can render it whereever or however you'd like to
  browser_instance b;
  browser::get().create( b, screen ); //automatically navigates to google.com
  //uncomment this to see the demo ui
  browser::get().navigate( b, "file:///C:/dev/gl_browser_gui/resources/ui/ui.html" ); //user our local GUI file

  //NOTE these are important bits here
  //use callbacks like these to handle js to cpp function calls
  //these will be called when you call the browser::update function

  //example js to cpp function call
  browser::get().register_callback( L"js_to_cpp", fun( [=]( std::wstring str )
  {
    wcout << str << endl;
    js::cpp_to_js( browser::get().get_last_callback_window(), L"cpp to js function call" );
  }, std::wstring() ) ); //setting the type here will make sure a functor with an argument will be created (omit this to create a functor wo/ an argument)

  //an example js to cpp function wo/ an argument
  browser::get().register_callback( L"dummy_function", fun( [=]()
  {
  } ) );

  //this gets called when the user calls window.open() (see above)
  //we create a new window
  //TODO: make sure these "managed" windows can be accessed by the user (ie. let the user get them to render them somewhere)
  browser::get().register_callback( L"open_window", fun( [=]( std::wstring str )
  {
    browser_instance& nw = *new browser_instance();
    browser::get().create( nw, screen, true ); //non-user-created window --> managed
  }, std::wstring() ) );

  //this gets called when the user calls resize_window()
  //this actually lets the GUI programmatically resize the window
  //TODO: add position change
  browser::get().register_callback( L"resize_window", fun( [=]( std::wstring str )
  {
    ivec2 pos = ivec2( the_window.getPosition().x, the_window.getPosition().y );
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    int w, h;
    std::string tmp = conv.to_bytes( str );
    sscanf( tmp.c_str(), "%d %d", &w, &h );
    the_window.setPosition( sf::Vector2<int>( pos.x, pos.y ) );
    the_window.setSize( sf::Vector2<unsigned>( w, h ) );
  }, std::wstring() ) );

  //this gets called when a user clicks on a <input type="file" /> element
  //the selected files are returned to a js callbacks function
  browser::get().register_callback( L"choose_file", fun( [=]( std::wstring str )
  {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::string tmp = conv.to_bytes( str );
    std::string title = tmp.substr( 0, tmp.rfind( " " ) );
    std::string mode = tmp.substr( tmp.rfind( " " ) + 1, std::string::npos );
    std::vector<std::string> filenames;

    if( mode == "of" )
      browser::get().select_file( filenames, title != "" ? title : "Open file", false, fileopen_type::OPEN );

    if( mode == "sf" )
      browser::get().select_file( filenames, title != "" ? title : "Select folder", false, fileopen_type::SELECT_FOLDER );

    if( mode == "om" )
      browser::get().select_file( filenames, title != "" ? title : "Open multiple file", true, fileopen_type::OPEN );

    if( mode == "sa" )
      browser::get().select_file( filenames, title != "" ? title : "Save as", true, fileopen_type::SAVE );

    //TODO the api doesn't say where to save the return value...
    //so we'll just call a js function...
    for( auto& s : filenames )
    {
      std::wstringstream ss;
      ss << L"filechooser_callback('" << conv.from_bytes( s ) << L"');";
      browser::get().execute_javascript( browser::get().get_last_callback_window(), ss.str() );
    }
  }, std::wstring() ) );

  /*
   * Set up the shaders
   */

  //NOTE: you should handle the rendering, so this should be custom code
  //load browser shader
  GLuint browser_shader = 0;
  load_shader( browser_shader, GL_VERTEX_SHADER, "../shaders/browser/browser.vs" );
  load_shader( browser_shader, GL_FRAGMENT_SHADER, "../shaders/browser/browser.ps" );

  /*
   * Handle events
   */

  //NOTE you should handle which browser window to send the events
  //example event handling
  auto event_handler = [&]( const sf::Event & ev )
  {
    switch( ev.type )
    {
      case sf::Event::MouseMoved:
      {
                                  vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

                                  browser::get().mouse_moved( b, mpos );

                                  break;
      }
      case sf::Event::TextEntered:
      {
                                   wchar_t txt[2];
                                   txt[0] = ev.text.unicode;
                                   txt[1] = '\0';
                                   browser::get().text_entered( b, txt );

                                   break;
      }
      case sf::Event::MouseButtonPressed:
      {
                                          if( ev.mouseButton.button == sf::Mouse::Left )
                                          {
                                            browser::get().mouse_button_event( b, sf::Mouse::Left, true );
                                          }
                                          else
                                          {
                                            browser::get().mouse_button_event( b, sf::Mouse::Right, true );
                                          }

                                          break;
      }
      case sf::Event::MouseButtonReleased:
      {
                                           if( ev.mouseButton.button == sf::Mouse::Left )
                                           {
                                             browser::get().mouse_button_event( b, sf::Mouse::Left, false );
                                           }
                                           else
                                           {
                                             browser::get().mouse_button_event( b, sf::Mouse::Right, false );
                                           }

                                           break;
      }
      case sf::Event::MouseWheelMoved:
      {
                                       browser::get().mouse_wheel_moved( b, ev.mouseWheel.delta * 100.0f );

                                       break;
      }
      case sf::Event::Resized:
      {
                               screen = uvec2( ev.size.width, ev.size.height );

                               glViewport( 0, 0, screen.x, screen.y );

                               browser::get().resize( b, screen );

                               break;
      }
      default:
        break;
    }
  };

  /*
   * Render
   */

  sf::Clock timer;
  timer.restart();

  while( run )
  {
    handle_events( event_handler );

    //update the browser
    browser::get().update();

    //render other stuff here
    glClear( GL_COLOR_BUFFER_BIT );

    //-----------------------------
    //render the UI
    //-----------------------------

    /**/

    //NOTE you decide how you'd like to render the browsers
    glDisable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glUseProgram( browser_shader );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, b.browser_texture );

    glBindVertexArray( ss_quad );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );

    /**/

    get_opengl_error();

    the_window.display();
  }

  browser::get().destroy( b );
  browser::get().shutdown();

  return 0;
}

//unimportant bits from here

template< class t >
void handle_events( const t& f )
{
  while( the_window.pollEvent( the_event ) )
  {
    if( the_event.type == sf::Event::Closed ||
        (
          the_event.type == sf::Event::KeyPressed &&
          the_event.key.code == sf::Keyboard::Escape
        )
      )
    {
      run = false;
    }

    f( the_event );
  }
}

void compile_shader( const char* text, const GLuint& program, const GLenum& type, const std::string& additional_str )
{
  GLchar infolog[INFOLOG_SIZE];

  GLuint id = glCreateShader( type );
  std::string str = text;
  str = additional_str + str;
  const char* c = str.c_str();
  glShaderSource( id, 1, &c, 0 );
  glCompileShader( id );

  GLint success;
  glGetShaderiv( id, GL_COMPILE_STATUS, &success );

  if( !success )
  {
    glGetShaderInfoLog( id, GET_INFOLOG_SIZE, 0, infolog );
    cerr << infolog << endl;
  }
  else
  {
    glAttachShader( program, id );
    glDeleteShader( id );
  }
}

void link_shader( const GLuint& shader_program )
{
  glLinkProgram( shader_program );

  GLint success;
  glGetProgramiv( shader_program, GL_LINK_STATUS, &success );

  if( !success )
  {
    GLchar infolog[INFOLOG_SIZE];
    glGetProgramInfoLog( shader_program, GET_INFOLOG_SIZE, 0, infolog );
    cout << infolog << endl;
  }

  glValidateProgram( shader_program );

  glGetProgramiv( shader_program, GL_VALIDATE_STATUS, &success );

  if( !success )
  {
    GLchar infolog[INFOLOG_SIZE];
    glGetProgramInfoLog( shader_program, GET_INFOLOG_SIZE, 0, infolog );
    cout << infolog << endl;
  }
}

GLuint create_quad( const vec3& ll, const vec3& lr, const vec3& ul, const vec3& ur )
{
  vector<float> vertices;
  vector<float> tex_coords;
  vector<unsigned> indices;
  GLuint vao = 0;
  GLuint vertex_vbo = 0, tex_coord_vbo = 0, index_vbo = 0;

  vertices.reserve( 4 * 3 );
  tex_coords.reserve( 4 * 2 );
  indices.reserve( 2 * 3 );

  indices.push_back( 0 );
  indices.push_back( 1 );
  indices.push_back( 2 );

  indices.push_back( 0 );
  indices.push_back( 2 );
  indices.push_back( 3 );

  /*vertices.push_back( vec3( -1, -1, 0 ) );
  vertices.push_back( vec3( 1, -1, 0 ) );
  vertices.push_back( vec3( 1, 1, 0 ) );
  vertices.push_back( vec3( -1, 1, 0 ) );*/
  vertices.push_back( ll.x );
  vertices.push_back( ll.y );
  vertices.push_back( ll.z );

  vertices.push_back( lr.x );
  vertices.push_back( lr.y );
  vertices.push_back( lr.z );

  vertices.push_back( ur.x );
  vertices.push_back( ur.y );
  vertices.push_back( ur.z );

  vertices.push_back( ul.x );
  vertices.push_back( ul.y );
  vertices.push_back( ul.z );

  tex_coords.push_back( 0 );
  tex_coords.push_back( 0 );

  tex_coords.push_back( 1 );
  tex_coords.push_back( 0 );

  tex_coords.push_back( 1 );
  tex_coords.push_back( 1 );

  tex_coords.push_back( 0 );
  tex_coords.push_back( 1 );

  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );

  glGenBuffers( 1, &vertex_vbo );
  glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo );
  glBufferData( GL_ARRAY_BUFFER, sizeof(float)* vertices.size(), &vertices[0], GL_STATIC_DRAW );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 3, GL_FLOAT, 0, 0, 0 );

  glGenBuffers( 1, &tex_coord_vbo );
  glBindBuffer( GL_ARRAY_BUFFER, tex_coord_vbo );
  glBufferData( GL_ARRAY_BUFFER, sizeof(float)* tex_coords.size(), &tex_coords[0], GL_STATIC_DRAW );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1, 2, GL_FLOAT, 0, 0, 0 );

  glGenBuffers( 1, &index_vbo );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)* indices.size(), &indices[0], GL_STATIC_DRAW );

  glBindVertexArray( 0 );

  return vao;
}

void load_shader( GLuint& program, const GLenum& type, const string& filename, const std::string& additional_str )
{
  ifstream f( filename );

  if( !f.is_open() )
  {
    cerr << "Couldn't load shader: " << filename << endl;
    return;
  }

  string str( ( istreambuf_iterator<char>( f ) ),
              istreambuf_iterator<char>() );

  if( !program ) program = glCreateProgram();

  compile_shader( str.c_str(), program, type, additional_str );
  link_shader( program );
}

void get_opengl_error( const bool& ignore )
{
  bool got_error = false;
  GLenum error = 0;
  error = glGetError();
  string errorstring = "";

  while( error != GL_NO_ERROR )
  {
    if( error == GL_INVALID_ENUM )
    {
      //An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: invalid enum...\n";
      got_error = true;
    }

    if( error == GL_INVALID_VALUE )
    {
      //A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: invalid value...\n";
      got_error = true;
    }

    if( error == GL_INVALID_OPERATION )
    {
      //The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: invalid operation...\n";
      got_error = true;
    }

    if( error == GL_STACK_OVERFLOW )
    {
      //This command would cause a stack overflow. The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: stack overflow...\n";
      got_error = true;
    }

    if( error == GL_STACK_UNDERFLOW )
    {
      //This command would cause a stack underflow. The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: stack underflow...\n";
      got_error = true;
    }

    if( error == GL_OUT_OF_MEMORY )
    {
      //There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.
      errorstring += "OpenGL error: out of memory...\n";
      got_error = true;
    }

    if( error == GL_TABLE_TOO_LARGE )
    {
      //The specified table exceeds the implementation's maximum supported table size.  The offending command is ignored and has no other side effect than to set the error flag.
      errorstring += "OpenGL error: table too large...\n";
      got_error = true;
    }

    error = glGetError();
  }

  if( got_error && !ignore )
  {
    cerr << errorstring;
    return;
  }
}