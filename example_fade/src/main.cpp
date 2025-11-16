#include "ofApp.h"
#include "ofMain.h"

int main(){
  // use GLFW window settings with GL 4.1
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(1920, 1080);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);
  
  ofRunApp(new ofApp());
}
