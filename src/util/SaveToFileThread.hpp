//
//  SaveToFileThread.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/10/2025.
//

#pragma once

#include "ofThread.h"
#include "ofPixels.h"
#include <string>

struct SaveToFileThread : public ofThread {
  static uint activeThreadCount;

  void save(const std::string& filepath_, ofFloatPixels&& pixels_);
  void threadedFunction();
  std::string filepath;
  ofFloatPixels pixels;
};
