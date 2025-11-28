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
#include <atomic>

struct SaveToFileThread : public ofThread {
  static std::atomic<uint32_t> activeThreadCount;

  static uint32_t getActiveThreadCount() { return activeThreadCount.load(std::memory_order_relaxed); }

  void save(const std::string& filepath_, ofFloatPixels&& pixels_);
  void threadedFunction();
  std::string filepath;
  ofFloatPixels pixels;
};
