#include "GLG3D/OSWindow.h"
namespace G3D {
    class CocoaWindow {
    public:
        static Vector2 primaryDisplaySize();
        static Vector2 virtualDisplaySize();
        static Vector2 primaryDisplayWindowSize();
        static int numDisplays();
        static OSWindow* create(const OSWindow::Settings &s);
    };

}