#include "ofApp.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(768, 768);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);

  ofRunApp(new ofApp());
}
