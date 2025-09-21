#include "ofApp.h"
#include "ofMain.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(1280, 1280);
  settings.windowMode = OF_WINDOW;
  ofCreateWindow(settings);

  ofRunApp(new ofApp());
}
