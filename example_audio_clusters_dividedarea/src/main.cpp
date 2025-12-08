#include "ofApp.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(1024, 768);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);

  ofRunApp(new ofApp());
}
