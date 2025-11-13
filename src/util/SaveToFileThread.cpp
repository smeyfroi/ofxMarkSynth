//
//  SaveToFileThread.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/10/2025.
//

#include "SaveToFileThread.hpp"
#include "ofxTinyEXR.h"
#include "ofLog.h"

uint SaveToFileThread::activeThreadCount = 0;

void SaveToFileThread::save(const std::string& filepath_, ofFloatPixels&& pixels_)
{
  pixels = pixels_;
  filepath = filepath_;
  startThread();
  activeThreadCount++;
}

void SaveToFileThread::threadedFunction() {
  ofLogNotice("SaveToFileThread") << "Saving drawing to " << filepath;
  
  ofxTinyEXR exrIO;
  bool saved = exrIO.savePixels(pixels, filepath);
  activeThreadCount--;
  if (!saved) ofLogError("SaveToFileThread") << "Failed to save EXR image";
  
  ofLogNotice("SaveToFileThread") << "Done saving drawing to " << filepath;
}
