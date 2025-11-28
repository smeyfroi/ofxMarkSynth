//
//  SaveToFileThread.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/10/2025.
//

#include "SaveToFileThread.hpp"
#include "ofxTinyEXR.h"
#include "ofLog.h"

std::atomic<uint32_t> SaveToFileThread::activeThreadCount{0};

void SaveToFileThread::save(const std::string& filepath_, ofFloatPixels&& pixels_)
{
  pixels = pixels_;
  filepath = filepath_;
  startThread();
}

void SaveToFileThread::threadedFunction() {
  SaveToFileThread::activeThreadCount.fetch_add(1, std::memory_order_relaxed);
  struct Guard {
    std::atomic<uint32_t>& c;
    ~Guard() { c.fetch_sub(1, std::memory_order_relaxed); }
  } guard{SaveToFileThread::activeThreadCount};

  ofLogNotice("SaveToFileThread") << "Saving drawing to " << filepath;

  ofxTinyEXR exrIO;
  bool saved = exrIO.savePixels(pixels, filepath);
  if (!saved) ofLogError("SaveToFileThread") << "Failed to save EXR image";

  ofLogNotice("SaveToFileThread") << "Done saving drawing to " << filepath;
}
