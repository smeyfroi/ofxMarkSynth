#include "ofApp.h"
#include "ofMain.h"

// Window sizes when not fullscreen
const int MAIN_WINDOW_WIDTH = 1600;
const int MAIN_WINDOW_HEIGHT = 1200;
const int GUI_WINDOW_WIDTH = 1200;
const int GUI_WINDOW_HEIGHT = 1200;

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
  settings.windowMode = OF_WINDOW;
  settings.decorated = true;
  settings.resizable = false;
  auto mainWindow = ofCreateWindow(settings);

  settings.setSize(GUI_WINDOW_WIDTH, GUI_WINDOW_HEIGHT);
  settings.decorated = true;
  settings.resizable = true;
  settings.title = "MarkSynth";
  settings.shareContextWith = mainWindow;
  auto guiWindow = ofCreateWindow(settings);

  auto mainApp = std::make_shared<ofApp>();
  mainApp->setGuiWindowPtr(guiWindow);
  ofAddListener(guiWindow->events().draw, mainApp.get(), &ofApp::drawGui);
  ofAddListener(guiWindow->events().keyPressed, mainApp.get(), &ofApp::keyPressedEvent); // needs adapter because keyPressed doesn't take an ofEventArgs& parameter
  ofRunApp(mainWindow, mainApp);
  ofRunMainLoop();
}
