//
//  TranslateMod.cpp
//  example_fade
//
//  Created by Steve Meyfroidt on 11/05/2025.
//

//#include "TranslateMod.hpp"
//#include "IntentMapping.hpp"
//
//
//namespace ofxMarkSynth {
//
//
//TranslateMod::TranslateMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
//: Mod { synthPtr, name, std::move(config) }
//{
//  translateShader.load();
//  
//  sinkNameIdMap = {
//    { "vec2", SINK_VEC2 }
//  };
//}
//
//void TranslateMod::initParameters() {
//  parameters.add(translateByParameter);
//}
//
//void TranslateMod::update() {
//  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
//  if (!drawingLayerPtrOpt) return;
//  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;
//
//  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
//  translateShader.render(*fboPtr, translation);
//}
//
//void TranslateMod::receive(int sinkId, const glm::vec2& v) {
//  switch (sinkId) {
//    case SINK_VEC2:
//      translateByParameter = v;
//      break;
//    default:
//      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
//  }
//}
//
//void TranslateMod::applyIntent(const Intent& intent, float strength) {
//  if (strength < 0.01f) return;
//  float magnitude = linearMap(intent.getEnergy(), 0.0f, 0.005f);
//  float angleOffset = intent.getChaos() * glm::two_pi<float>();
//  float angle = angleOffset;
//  glm::vec2 translation { magnitude * std::cos(angle), magnitude * std::sin(angle) };
//  translateByParameter.set(translation);
//}
//
//
//} // ofxMarkSynth
