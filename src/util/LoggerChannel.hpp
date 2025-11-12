//
//  LoggerChannel.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 12/11/2025.
//

#pragma once

#include "ofLog.h"
#include <mutex>



namespace ofxMarkSynth {



class LoggerChannel : public ofBaseLoggerChannel {
  
public:
  struct LogMessage {
    ofLogLevel level;
    std::string message;
  };
  
  void log(ofLogLevel level, const std::string &module, const std::string &message) override {
    std::lock_guard<std::mutex> lock(mutex);
    logs.push_back({ level, "[" + module + "] " + message });
  }
  
//  void log(ofLogLevel level, const std::string &message) override {
//    std::lock_guard<std::mutex> lock(mutex);
//    logs.push_back({ level, message });
//  }
  
  std::vector<LogMessage> getLogs() const {
    std::lock_guard<std::mutex> lock(mutex);
    return logs;
  }
  
  void clear() {
    std::lock_guard<std::mutex> lock(mutex);
    logs.clear();
  }
  
private:
  mutable std::mutex mutex;
  std::vector<LogMessage> logs;
};



} // namespace ofxMarkSynth
