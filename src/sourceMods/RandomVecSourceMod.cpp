//
//  RandomVecSourceMod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "RandomVecSourceMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



RandomVecSourceMod::RandomVecSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, short vecDimensions_)
: Mod { synthPtr, name, std::move(config) },
vecDimensions { vecDimensions_ }
{
  sourceNameIdMap = {
    { "Vec2", SOURCE_VEC2 },
    { "Vec3", SOURCE_VEC3 },
    { "Vec4", SOURCE_VEC4 }
  };
  
  registerControllerForSource(vecsPerUpdateParameter, vecsPerUpdateController);
}

void RandomVecSourceMod::initParameters() {
  parameters.add(vecsPerUpdateParameter);
  parameters.add(agencyFactorParameter);
}

float RandomVecSourceMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void RandomVecSourceMod::update() {
  syncControllerAgencies();
  vecsPerUpdateController.update();
  
  vecCount += vecsPerUpdateController.value;
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
        ofLogError("RandomVecSourceMod") << "Update with vecDimensions " << vecDimensions;
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

void RandomVecSourceMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  im.D().exp(vecsPerUpdateController, strength, 2.0f);
}



} // ofxMarkSynth
