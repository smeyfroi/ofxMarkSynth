//
//  RandomVecSourceMod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "RandomVecSourceMod.hpp"


namespace ofxMarkSynth {


RandomVecSourceMod::RandomVecSourceMod(const std::string& name, const ModConfig&& config, short vecDimensions_)
: Mod { name, std::move(config) },
vecDimensions { vecDimensions_ }
{}

void RandomVecSourceMod::initParameters() {
  parameters.add(vecsPerUpdateParameter);
}

void RandomVecSourceMod::update() {
  vecCount += vecsPerUpdateParameter;
  int vecsToCreate = std::floor(vecCount);
  vecCount -= vecsToCreate;
  if (vecsToCreate == 0) return;

  for (int i = 0; i < vecsToCreate; i++) {
    switch (vecDimensions) {
      case SOURCE_VEC2:
        emit(SOURCE_VEC2, createRandomVec2());
        break;
      case SOURCE_VEC3:
        emit(SOURCE_VEC3, createRandomVec3());
        break;
      case SOURCE_VEC4:
        emit(SOURCE_VEC4, createRandomVec4());
        break;
      default:
        ofLogError() << "update in " << typeid(*this).name() << " with vecDimensions " << vecDimensions;
        break;
    }
  }
}

const glm::vec2 RandomVecSourceMod::createRandomVec2() const {
  return glm::vec2 { ofRandom(), ofRandom() };
}

const glm::vec3 RandomVecSourceMod::createRandomVec3() const {
  return glm::vec3 { ofRandom(), ofRandom(), ofRandom() };
}

const glm::vec4 RandomVecSourceMod::createRandomVec4() const {
  return glm::vec4 { ofRandom(), ofRandom(), ofRandom(), ofRandom() };
}


} // ofxMarkSynth
