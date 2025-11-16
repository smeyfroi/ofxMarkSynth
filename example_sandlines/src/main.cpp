#include "ofApp.h"

int main(){
  // use GLFW window settings with GL 4.1
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(768, 768);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);

	ofRunApp(new ofApp());
}
