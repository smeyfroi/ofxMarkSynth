#include "ofApp.h"
#include "ofMain.h"

int main(){
  // GLFW window with GL 4.1
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(1024, 768);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);

  ofRunApp(new ofApp());
}
