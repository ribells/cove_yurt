/*
 * test.cpp
 *
 * Adapted from vrg3d_demo.cpp for COVE integration.
 * (see: http://cove.ocean.washington.edu/
 */

#include <vrg3d/VRG3D.h>
#include <GL/glut.h>
#include "timer.h"
#include "timeWidget.h"
#include "idv/world.h"
#include "idv/gl_draw.h"

using namespace G3D;

extern void init_cove(), draw_cove(float x, float y, float z, float yaw, float pitch, float roll);

/** This is a sample VR application using the VRG3D library.  Two key
    methods are filled in here: doGraphics() and doUserInput().  The
    code in these methods demonstrates how to draw graphics and
    respond to input from the mouse, the keyboard, 6D trackers, and VR
    wand buttons.
*/

class MyVRApp : public VRApp
{
public:
  MyVRApp(const std::string &mySetup) : VRApp()
  {
     // initialize the VRApp
     Log  *demoLog = new Log("demo-log.txt");
     init(mySetup, demoLog);

     _mouseToTracker = new MouseToTracker(getCamera(), 2);

     // Initialize the coordinate frame for the display.
     _virtualToRoomSpace = CoordinateFrame();

     /* The default starting point with Axial earthquake data has the eye level at
      * the x, y, z origin, which is at 130 degrees West and 46 degrees North off the
      * coast of Oregon. The origin is confusing for the viewer on startup, and renders
      * poorly too. We move the virtual space down a few km for a more sensible view. */

     // IF YOU WANT TO USE THE DEFAULT VIEWPOINT FROM THE COVE CAMERA, USE:
     // Vec3d look_at = g_Draw.getCamera().getLookAt();
     // _virtualToRoomSpace = CoordinateFrame(Vector3(look_at[0], look_at[1], look_at[2])) * _virtualToRoomSpace;

     // OTHERWISE, WRITE YOUR OWN STARTING VIEWPOINT HERE (location and then orientation):
     _virtualToRoomSpace = CoordinateFrame(Vector3(130.0,-0.21,46.0)) * _virtualToRoomSpace;
	 _virtualToRoomSpace = CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(0.0))) * _virtualToRoomSpace;
     float x = 0.0;
     float y = 0.0;
     float z = 0.0;
     float yaw = 0.0;
     float pitch = 0.0;
     float roll = 0.0;
     _virtualToRoomSpace.getXYZYPRDegrees(x, y, z, yaw, pitch, roll);
     //if you choose a different viewpoint, update the COVE camera to be the same:
     g_Draw.SetCameraPosition(Vec3f(x, y, z), Vec3f(yaw, pitch, roll));

     // This is the background color for the world:
     _clearColor = Color3(0.0, 0.0, 0.0);
     //_clearColor = Color3(0.20, 0.20, 0.55);
     // The actual visual content for the world loads from another code file:
     init_cove();
  }

  virtual ~MyVRApp() {}

  void doUserInput(Array<VRG3D::EventRef> &events)
  {
    /* MouseToTracker is a really helpful class for testing out VR
     * interactions from the desktop.  This call makes it respond to
     * mouse events and generate new events as if it were a 6DOF
     * tracking device.  We add the new events to the event queue and
     * process them as usual. */

    static double joystick_x = 0.0;
    static double joystick_y = 0.0;
    static bool toggled = false;

    Array<VRG3D::EventRef> newEvents;
    _mouseToTracker->doUserInput(events, newEvents);
    events.append(newEvents);

    for (int i = 0; i < events.size(); i++) {
      if (events[i]->getName() == "kbd_ESC_down") {
	    // Exit the program.
        while(glGetError() != GL_NO_ERROR) {
	        std::cout<<"Flushing gl errors"<<std::endl;
	    }
        exit(0);
      }

      // Save all the tracker events that come in so we can use them in the doGraphics routine
      else if (endsWith(events[i]->getName(), "_Tracker")) {
        if (_trackerFrames.containsKey(events[i]->getName())) {
        	_trackerFrames[events[i]->getName()] = events[i]->getCoordinateFrameData();
        } else {
        	_trackerFrames.set( events[i]->getName(), events[i]->getCoordinateFrameData() );
        }
      // Respond to events to do some simple navigation
      } else if (events[i]->getName() == "kbd_H_down") { //go to COVE camera's look_at position
    	  Vec3d look_at = g_Draw.getCamera().getLookAt();
    	  _virtualToRoomSpace = CoordinateFrame(Vector3(look_at[0], look_at[1], look_at[2])) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_LEFT_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(0.5))) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_RIGHT_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(-0.5))) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_UP_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Matrix3::fromAxisAngle(Vector3(1,0,0), toRadians(0.5))) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_DOWN_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Matrix3::fromAxisAngle(Vector3(1,0,0), toRadians(-0.5))) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_W_down") {
    	  if(get_time_ui_active_index()>=0) {
    		  set_time_ui_value(-1);
    	  } else {
    		  _virtualToRoomSpace = CoordinateFrame(Vector3(0,0,0.01)) * _virtualToRoomSpace;
    	  }
      } else if (events[i]->getName() == "kbd_S_down") {
    	  if(get_time_ui_active_index()>=0) {
    		  set_time_ui_value(1);
    	  } else {
    		  _virtualToRoomSpace = CoordinateFrame(Vector3(0,0,-0.01)) * _virtualToRoomSpace;
    	  }
      } else if (events[i]->getName() == "kbd_A_down") {
    	  if(get_time_ui_active_index()>=0) {
    		  get_previous_ui_active_index();
    	  } else {
    		  _virtualToRoomSpace = CoordinateFrame(Vector3(-0.01,0,0)) * _virtualToRoomSpace;
    	  }
      } else if (events[i]->getName() == "kbd_D_down") {
    	  if(get_time_ui_active_index()>=0) {
    		  get_next_ui_active_index();
    	  } else {
    		  _virtualToRoomSpace = CoordinateFrame(Vector3(0.01,0,0)) * _virtualToRoomSpace;
    	  }
      } else if (events[i]->getName() == "kbd_I_down") {
    	  g_World.getTimeLine().setSlower();
      } else if (events[i]->getName() == "kbd_K_down") {
    	  g_World.getTimeLine().setFaster();
      } else if (events[i]->getName() == "kbd_R_down") {
    	  g_World.getTimeLine().setPlay(true, true);
      } else if (events[i]->getName() == "kbd_F_down") {
    	  g_World.getTimeLine().setPlay(true, false);
      } else if (events[i]->getName() == "kbd_T_down") {
    	  if(get_time_ui_active_index() < 0) {
    		  show_layout_timeline(true);
    	  } else {
    		  show_layout_timeline(false);
    	  }
      } else if (events[i]->getName() == "kbd_SHIFT_UP_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Vector3(0,0.01,0)) * _virtualToRoomSpace;
      } else if (events[i]->getName() == "kbd_SHIFT_DOWN_down") {
    	  _virtualToRoomSpace = CoordinateFrame(Vector3(0,-0.01,0)) * _virtualToRoomSpace;
      // Some printouts for other events, just to show how to access other types of event data
      } else if (events[i]->getName() == "kbd_SPACE_down") {
            if (!toggled) {
              _virtualToRoomSpace.translation = 75.0f * Vector3(0,0,1);
              toggled = !toggled;
            } else {
              _virtualToRoomSpace.translation = Vector3(0,0,0);
              toggled = !toggled;
            }
      } else if (events[i]->getName() == "Wand_Btn1_down") {
    	  	  //cout << "Wand btn 1 pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn2_down") {
        	  //cout << "Wand btn 2 pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn3_down") {
        	  //cout << "Wand btn 3 pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn4_down") {
        	  //cout << "Wand btn 4 pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn6_down") {
        	  //cout << "Wand btn 6 pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn5_down") {
        	  //cout << "Wand joystick btn pressed." << endl;
          } else if (events[i]->getName() == "Wand_Btn6_down") {
        	  //cout << "Wand trigger btn pressed." << endl;
          } else if (events[i]->getName() == "Wand_Joystick_X") {
        	  //cout << "Wand Joystick X = " << events[i]->get1DData() << endl;
        	  joystick_x = events[i]->get1DData();
        	  //-1,1
          } else if (events[i]->getName() == "Wand_Joystick_Y") {
        	  //cout << "Wand Joystick Y = " << events[i]->get1DData() << endl;
        	  joystick_y = events[i]->get1DData();
        	  //-1,1
          } else if (events[i]->getName() == "Mouse_Pointer") {
        	static Vector2 lastPos;
            if (events[i]->get2DData() != lastPos) {
            	//  cout << "New mouse position = " << events[i]->get2DData() << endl;
            	lastPos = events[i]->get2DData();
            }
          } else if (events[i]->getName() == "Mouse_Left_Btn_down") {
        	  //cout << "Mouse left btn pressed at position " << events[i]->get2DData() << endl;
          } else if (beginsWith(events[i]->getName(), "kbd_")) {
        	  //cout << "Keyboard event 1: " << events[i]->getName() << endl;
        	  //cout << getCamera()->getHeadFrame() << endl;
        	  //cout << _virtualToRoomSpace << endl;
          } else if (events[i]->getName() == "SpaceNav_Trans") {
        	  //cout << "Keyboard event 2: " << events[i]->getName() << events[i]->get3DData() << endl;
          } else if (events[i]->getName() == "SpaceNav_Rot") {
        	  //cout << "Keyboard event 3: " << events[i]->getName() << events[i]->get3DData() << endl;
          } else if (beginsWith(events[i]->getName(), "TNG_An")) {
        	  //cout << events[i]->getName() << "  " << events[i]->get1DData() << endl;
          } else if (events[i]->getName() == "SynchedTime") {
        	  continue;
          } else {
        	  /* This will print out the names of all events, but can be too
        	   * much if you are getting several tracker updates per frame.
        	   * Uncomment this to see everything: */
        	  //cout << events[i]->getName() << endl;
          }

          // For debugging tracker coordinate systems, it can be useful to print out
          // tracker positions, like this:
          if (events[i]->getName() == "Test_Tracker") {
        	  //cout << events[i]->getName() << " " << events[i]->getCoordinateFrameData().translation << endl;
          }

          // Rotate
          if (fabs(joystick_x) > 0.01) {
            //fprintf(stderr, "Joystick x: %lf\n", joystick_x);
            double angle = M_PI / 180.0 * joystick_x;
            angle /= 5.0;
            CoordinateFrame rotation = CoordinateFrame(Matrix3::fromEulerAnglesXYZ(0, angle, 0));
            _virtualToRoomSpace = rotation * _virtualToRoomSpace;
          }

          // Translate
          if (fabs(joystick_y) > 0.0 && _trackerFrames.containsKey("Wand_Tracker") == true) {
            if (joystick_y < 0) {
              _virtualToRoomSpace.translation = 75.0f * Vector3(0,0,1);
            } else {
              _virtualToRoomSpace.translation = Vector3(0,0,0);
              //_virtualToRoomSpace = CoordinateFrame(Vector3(0, 0, 0.25f*joystick_y)) * _virtualToRoomSpace;
              //_virtualToRoomSpace.translation -= .25f * joystick_y * _trackerFrames[string("Wand_Tracker")].lookVector();
            }
          }
      }
      float x = 0.0;
      float y = 0.0;
      float z = 0.0;
      float yaw = 0.0;
      float pitch = 0.0;
      float roll = 0.0;
      _virtualToRoomSpace.getXYZYPRDegrees(x, y, z, yaw, pitch, roll);
      g_Draw.SetCameraPosition(Vec3f(x, y, z), Vec3f(yaw, pitch, roll));
      g_World.getTerrain().setNeedTerrainUpdate();
  }

  void doGraphics(RenderDevice *rd)
  {
    // Load a font for the fps display, findVRG3DDataFile looks first
    // in the current directory and then in $G/src/VRG3D/share/

    while(glGetError() != GL_NO_ERROR) {
    	std::cout<<"Flushing gl errors"<<std::endl;
    }

    if (_font.isNull()) {
    	std::string fontfile = VRApp::findVRG3DDataFile("eurostyle.fnt");
    	if ( FileSystem::exists( fontfile )) {
    		std::cout << fontfile << std::endl;
    		_font = GFont::fromFile( fontfile );
    	}
    }

    Array<std::string> trackerNames = _trackerFrames.getKeys();

    for (int i=0;i<trackerNames.size();i++) {
    	CoordinateFrame trackerFrame = _trackerFrames[trackerNames[i]];

        // Draw laser pointer.
        if (trackerNames[i] == "Wand_Tracker") {
            Vector3 lookVector = trackerFrame.lookVector();

            glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);

            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glLineWidth(2.0f);
            glColor4f(1.0f,0.0f,0.0f,1.0f);
            glBegin(GL_LINES);
            glVertex3f(trackerFrame.translation.x,
                       trackerFrame.translation.y,
                       trackerFrame.translation.z);
            glVertex3f(trackerFrame.translation.x + 6.0 * lookVector.x,
                       trackerFrame.translation.y + 6.0 * lookVector.y,
                       trackerFrame.translation.z + 6.0 * lookVector.z);
            glEnd();
            glPopAttrib();
       }
    }

    /* The tracker frames above are drawn with the object to world
     * matrix set to the identity because tracking data comes into the
     * system in the Room Space coordinate system. Room Space is tied
     * to the dimensions of the room and the projection screen within
     * the room, thus it never changes as your program runs. However,
     * it is often convenient to move objects around in a virtual
     * space that can change relative to the screen. For these
     * objects, we put a virtual to room space transform on the OpenGL
     * matrix stack before drawing them, as is done here: */

    rd->disableLighting();
    rd->pushState();
    rd->setObjectToWorldMatrix(_virtualToRoomSpace);

    /* Parameters for our light, including color and position */
    GLfloat ambient[] = {0.0, 0.0, 0.0, 1.0};
    GLfloat diffuse[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat position[] = {0.0, 3.0, 3.0, 0.0};
    GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
    GLfloat local_view[] = {0.0};

    glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT0, GL_POSITION, position);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    /*  These functions change how the object gets drawn */
    glEnable (GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    //glEnable(GL_CULL_FACE);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);

    // Use these if you plan to scale an object
    //glEnable (GL_AUTO_NORMAL);
    //glEnable (GL_NORMALIZE);

    // If you want to do alpha blending:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use G3D::Draw to draw a simple geometry of the axes at the origin of Virtual Space:
    //Draw::axes( CoordinateFrame(), rd, Color3::red(), Color3::green(), Color3::blue(), 1.25 );

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_COLOR_MATERIAL);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glDisable( GL_COLOR_MATERIAL );

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glDisable(GL_LIGHTING);

    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
    float yaw = 0.0;
    float pitch = 0.0;
    float roll = 0.0;
    _virtualToRoomSpace.getXYZYPRDegrees(x, y, z, yaw, pitch, roll);

    //All our drawing objects are from within the COVE program parts we've incorporated
    draw_cove(x, y, z, yaw, pitch, roll);
    rd->popState();
  }

protected:
  Table<std::string, CoordinateFrame> _trackerFrames;
  GFontRef          _font;
  MouseToTrackerRef _mouseToTracker;
  CoordinateFrame   _virtualToRoomSpace;
};

int main( int argc, char **argv )
{
  std::string setupStr; // which of the known VR setups to start
  MyVRApp *app;			// my application to run within VR

  if (argc >= 2) {
	  setupStr = std::string(argv[1]);
  }

  //START SETTING UP THE COVE ENVIRONMENT (TO DO: use a configuration file for setup outside of compile)
  bool	bRunning = false;
  bool  bRunLocal = true;
  cout << "Initializing COVE\n";
  if(bRunLocal) {
	  g_Env.m_CurFilePath = "/home/ribells/workspace/test/Debug/";
  } else {
	  g_Env.m_CurFilePath = "/users/guest461/test/Debug/";
  }
  g_Env.m_AppPath = g_Env.m_CurFilePath + "datasvr";
  g_Set.m_StartupFile = g_Env.m_CurFilePath + "datasvr/worlds/Earthquakes.cov";
  g_Env.m_LocalCachePath = "cove_temp";
  cout << "Application folder is " + g_Env.m_AppPath + "\n";
  cout << "Local data folder is " + g_Env.m_AppPath + "\n";
  cout << "Local cache folder is " + g_Env.m_LocalCachePath + "\n";
  cout << "Data server is " + g_Env.m_COVEServerPath + "\n";

  removedir(g_Env.m_LocalCachePath);
  makedir(g_Env.m_LocalCachePath);

  //initialize the GL space
  g_Draw.initState();

  if (!initSceneManager()) {
  	cout << "\nProblems initializing OpenGL functions. Application must exit\n";
  } else {
  	cout << "\nScene Manager loaded and ready for action.\n";
  }

  cout << "Python scripting is " + (string) (g_Set.m_PythonAvailable ? "" : "not ") + "available\n";

  g_Set.m_Projection = PROJ_GLOBE;
  g_World.cleanDBFileList();

  if (!g_World.readCoveFile(g_Set.m_StartupFile)) {
  		cout << "\nCannot read most recent COVE file: " + g_Set.m_StartupFile;
  		cout <<	"\nOpening default workspace " + g_Env.m_DefaultStartFile;
  }

  //init_new_world(); //from COVE main.cpp - not needed here?
  g_Set.m_UpdataLayerTree = true;

  //only start up world through VRPN - to get time synchronization between nodes
  g_World.getTimeLine().setPlay(false, false);

  bRunning = true;

  // This opens up the graphics window, and starts connections to
  // input devices, but doesn't actually start rendering yet.
  app = new MyVRApp(setupStr);

  // This starts the rendering/input processing loop
  app->run();

  return 0;
}
////////////////////  end  common/vrg3d/demo/vrg3d_demo.cpp  ///////////////////
