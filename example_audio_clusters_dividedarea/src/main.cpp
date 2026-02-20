#include "ofApp.h"
#include "ofMain.h"

const int MAIN_WINDOW_WIDTH = 1024;
const int MAIN_WINDOW_HEIGHT = 768;
const int GUI_WINDOW_WIDTH = 800;
const int GUI_WINDOW_HEIGHT = 1000;

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4, 1);
  settings.setSize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
  settings.windowMode = OF_WINDOW;
  settings.resizable = true;
  auto mainWindow = ofCreateWindow(settings);

  settings.setSize(GUI_WINDOW_WIDTH, GUI_WINDOW_HEIGHT);
  settings.resizable = true;
  settings.title = "gui";
  settings.shareContextWith = mainWindow;
  auto guiWindow = ofCreateWindow(settings);

  auto mainApp = std::make_shared<ofApp>();
  mainApp->setGuiWindowPtr(guiWindow);
  ofAddListener(guiWindow->events().draw, mainApp.get(), &ofApp::drawGui);
  ofAddListener(guiWindow->events().keyPressed, mainApp.get(), &ofApp::keyPressedEvent);
  ofRunApp(mainWindow, mainApp);
  ofRunMainLoop();
}
