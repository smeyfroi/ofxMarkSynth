//
//  TimeStringUtil.h
//  ofxMarkSynth
//
//  Utility for parsing time strings in "MM:SS" format.
//

#pragma once

#include <string>
#include "ofMain.h"

namespace ofxMarkSynth {



/// Parse a time string in "MM:SS" format to seconds.
/// Returns 0 and logs an error on parse failure.
inline int parseTimeStringToSeconds(const std::string& timeStr) {
  if (timeStr.empty()) {
    return 0;
  }
  
  auto colonPos = timeStr.find(':');
  if (colonPos == std::string::npos) {
    ofLogError("TimeStringUtil") << "Invalid time format (expected MM:SS): " << timeStr;
    return 0;
  }
  
  try {
    int minutes = std::stoi(timeStr.substr(0, colonPos));
    int seconds = std::stoi(timeStr.substr(colonPos + 1));
    if (minutes < 0 || seconds < 0 || seconds >= 60) {
      ofLogError("TimeStringUtil") << "Invalid time values (minutes >= 0, 0 <= seconds < 60): " << timeStr;
      return 0;
    }
    return minutes * 60 + seconds;
  } catch (const std::exception& e) {
    ofLogError("TimeStringUtil") << "Failed to parse time string '" << timeStr << "': " << e.what();
    return 0;
  }
}



} // namespace ofxMarkSynth
