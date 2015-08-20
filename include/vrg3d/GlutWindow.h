//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
 @file GlutWindow.h
 @author Morgan McGuire and Dan Keefe
 */
#ifndef GLUTWINDOW_H
#define GLUTWINDOW_H

#include <G3D/G3D.h>
#include <GLG3D/GLG3D.h>

#if G3D_VER < 60300
    #error Requires G3D 6.03
#endif

#define GLUT_API_VERSION 4

#ifdef G3D_OSX
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


/**
 GWindow that uses the glut API http://www.opengl.org/resources/libraries/glut/spec3/spec3.html.

  Not supported by GlutWindow:
  <UL>
   <LI> Joysticks
   <LI> Gamma ramp
   <LI> Window Icons
   <LI> Input capture
  </UL>

 Additionally, Glut does not support all keystrokes (e.g. the key-down event for a modifier
 cannot be detected, scan-codes are not available) so GlutWindow fails to report some keyboard
 events.

 @cite Glut by Mark Kilgard, ported to Windows by Nate Robbins
 */
// The public API of this implementation allows only one window because it does
// not provide a mechanism for switching active windows/contexts.  However,
// internally it has been coded so that this will be a simple change.
class GlutWindow : public G3D::GWindow {
private:

    /** Has glutInit been called? (only allowed once per program)*/
    static bool             glutInitialized;

    /** Underlying glut window handle */
    int                     glutWindowHandle;

    int                     _mouseHideCount;

    std::string             _windowTitle;

    G3D::Rect2D                  _dimensions;

    G3D::GWindow::Settings       settings;

    G3D::Queue<G3D::GEvent>           eventQueue;

    G3D::Vector2                 mouse;

    G3D::uint8                   mouseButtons;

    /** Where to send GLUT events */
    static GlutWindow*      currentGlutWindow;

    /** Adds an event to the queue-- is synchronized! */
    void postEvent(G3D::GEvent& evt);

    // Glut callbacks:
    static void g_reshape(int width, int height);
    static void g_keyboard(unsigned char c, int x, int y);
    static void g_keyboardspecial(int key, int x, int y);
    static void g_keyboardup(unsigned char c, int x, int y);
    static void g_keyboardspecialup(int key, int x, int y);
    static void g_mousemotion(int x, int y);
    static void g_mousebtn(int b, int s, int x, int y);
    static void g_draw();

public:

    GlutWindow(const G3D::GWindow::Settings& s);
    virtual ~GlutWindow();
    virtual void getSettings(G3D::GWindow::Settings& settings) const;
    virtual int width() const;
    virtual int height() const;
    virtual G3D::Rect2D dimensions() const;
    virtual void setDimensions(const G3D::Rect2D& dims);
    virtual void setPosition(int x, int y);
    virtual bool hasFocus() const;
    virtual std::string getAPIVersion() const;
    virtual std::string getAPIName() const;
    virtual void setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp);
    virtual void setCaption(const std::string& caption);
    virtual int numJoysticks() const;
    virtual std::string joystickName(unsigned int sticknum);
    virtual std::string caption();
    virtual void setIcon(const G3D::GImage& image);
    virtual void swapGLBuffers();
    virtual void notifyResize(int w, int h);
    virtual bool pollEvent(G3D::GEvent& e);

    virtual bool requiresMainLoop() const {
        return true;
    }

    virtual void runMainLoop();
    virtual void setRelativeMousePosition(double x, double y);
    virtual void setRelativeMousePosition(const G3D::Vector2& p);
    virtual void getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const;
    virtual void getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const;
    virtual void getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const;
    virtual void getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button);
    virtual void setInputCapture(bool c);
    virtual bool inputCapture() const;

    virtual bool mouseVisible() const;
    virtual void setMouseVisible(bool v);
    virtual void incMouseHideCount();
    virtual void decMouseHideCount();

    virtual void getDroppedFilenames(G3D::Array<std::string>&);
};


#endif
