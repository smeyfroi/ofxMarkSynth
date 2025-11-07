//
//  Parameter.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "Parameter.hpp"



namespace ofxMarkSynth {



void addFlattenedParameterGroup(ofParameterGroup& parentGroup, const ofParameterGroup& groupToFlatten) {
  for (const auto& param : groupToFlatten) {
    if (param->type() == typeid(ofParameterGroup).name()) {
      const ofParameterGroup& subGroup = param->cast<ofParameterGroup>();
      addFlattenedParameterGroup(parentGroup, subGroup);
    } else {
      parentGroup.add(*param);
    }
  }
}



} // namespace ofxMarkSynth
