//
//  NodeRenderUtil.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 12/11/2025.
//

#include "NodeRenderUtil.hpp"
#include "ImGuiUtil.hpp"
#include <cstdio>



namespace ofxMarkSynth {
namespace NodeRenderUtil {

// Static mono font for tooltips
static ImFont* monoFont = nullptr;
// Optional external tooltip map for sliders (e.g. layer descriptions)
static const std::unordered_map<std::string, std::string>* externalTooltipMap = nullptr;
 
void setMonoFont(ImFont* font) {
  monoFont = font;
}
 
void setLayerTooltipMap(const std::unordered_map<std::string, std::string>* tooltipMap) {
  externalTooltipMap = tooltipMap;
}
 
 
void drawVerticalSliders(ofParameterGroup& paramGroup) {
  const std::vector<std::shared_ptr<ofParameter<bool>>> empty;
  drawVerticalSliders(paramGroup, empty);
}

void drawVerticalSliders(ofParameterGroup& paramGroup,
                         const std::vector<std::shared_ptr<ofParameter<bool>>>& toggleParams) {
  if (paramGroup.size() == 0) return;
  
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8)); // tighter spacing
  const ImVec2 sliderSize(24, 140);
  const float colW = sliderSize.x + 0.0f;   // column width (slider + padding)
  const float colH = sliderSize.y + 28.0f;   // add room for label below
  
  if (ImGui::BeginTable(paramGroup.getName().c_str(), paramGroup.size(),
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
    
    for (int i = 0; i < paramGroup.size(); ++i) {
      ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, colW);
    }
    ImGui::TableNextRow();
    
    for (int i = 0; i < paramGroup.size(); ++i) {
      const auto& name = paramGroup[i].getName();
      std::shared_ptr<ofParameter<bool>> toggleParam;
      if (i < static_cast<int>(toggleParams.size())) {
        toggleParam = toggleParams[i];
      }
      
      ImGui::TableSetColumnIndex(i);
      ImGui::PushID(i);
      
      ImGui::BeginGroup();
      // center slider within the fixed column
      float xPad = (colW - sliderSize.x) * 0.5f;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPad);
      
      // copy current value to a local so VSliderFloat can edit it by pointer
      float v = paramGroup[i].cast<float>().get();
      if (ImGui::VSliderFloat("##v", sliderSize, &v, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoRoundToFormat)) {
        paramGroup[i].cast<float>().set(v);
      }
      if (externalTooltipMap && externalTooltipMap->contains(name)) {
        ImGui::SetItemTooltip("%s", externalTooltipMap->at(name).c_str());
      } else {
        ImGui::SetItemTooltip("%s", name.c_str());
      }
      
      // Optional pause toggle directly under the slider
      if (toggleParam) {
        float checkSize = ImGui::GetFrameHeight();
        float xPadCheck = (colW - checkSize) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPad + xPadCheck);
        bool b = toggleParam->get();
        if (ImGui::Checkbox(("##pause_" + name).c_str(), &b)) {
          toggleParam->set(b);
        }
        ImGui::SetItemTooltip("Pause layer %s", name.c_str());
        // Reset X so label below is still centered in column
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPadCheck);
      }
      
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPad);
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + colW);
      ImGui::TextWrapped("%s", name.substr(0, 3).c_str());
      ImGui::PopTextWrapPos();
      ImGui::EndGroup();
      
      ImGui::PopID();
    }
    
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

constexpr float sliderWidth = 200.0f;

// Helper to add tooltip showing component breakdown and final value for controlled parameters
void addControllerTooltip(const ModPtr& modPtr, const std::string& paramName) {
  if (modPtr->sourceNameControllerPtrMap.contains(paramName)) {
    auto* controllerPtr = modPtr->sourceNameControllerPtrMap.at(paramName);
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      if (monoFont) ImGui::PushFont(monoFont);
      ImGui::TextUnformatted(controllerPtr->getFormattedValue().c_str());
      if (monoFont) ImGui::PopFont();
      ImGui::EndTooltip();
    }
  }
}

void addParameter(const ModPtr& modPtr, ofParameter<int>& parameter) {
  const auto& name = parameter.getName();
  int value = parameter.get();
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderInt(("##" + name).c_str(), &value, parameter.getMin(), parameter.getMax())) {
    parameter.set(value);
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<float>& parameter) {
  const auto& name = parameter.getName();
  float value = parameter.get();
  
  ImGui::PushItemWidth(sliderWidth);
  const float r = parameter.getMax() - parameter.getMin();
  const char* fmt = (r <= 0.01f) ? "%.5f" : (r <= 0.1f) ? "%.4f" : (r <= 1.0f) ? "%.3f" : "%.2f";
  if (ImGui::SliderFloat(("##" + name).c_str(), &value, parameter.getMin(), parameter.getMax(), fmt, ImGuiSliderFlags_NoRoundToFormat)) {
    parameter.set(value);
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<ofFloatColor>& parameter) {
  const auto& name = parameter.getName();
  ofFloatColor color = parameter.get();
  float colorArray[4] = { color.r, color.g, color.b, color.a };
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::ColorEdit4(("##" + name).c_str(), colorArray, ImGuiColorEditFlags_Float)) {
    parameter.set(ofFloatColor(colorArray[0], colorArray[1], colorArray[2], colorArray[3]));
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<glm::vec2>& parameter) {
  const auto& name = parameter.getName();
  glm::vec2 value = parameter.get();
  float valueArray[2] = { value.x, value.y };
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderFloat2(("##" + name).c_str(), valueArray, parameter.getMin().x, parameter.getMax().x, "%.2f", ImGuiSliderFlags_NoRoundToFormat)) {
    parameter.set(glm::vec2(valueArray[0], valueArray[1]));
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<bool>& parameter) {
  const auto& name = parameter.getName();
  bool value = parameter.get();
  if (ImGui::Checkbox(("##" + name).c_str(), &value)) {
    parameter.set(value);
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<std::string>& parameter) {
  const auto& name = parameter.getName();
  const auto current = parameter.get();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s", current.c_str());
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::InputText(("##" + name).c_str(), buf, sizeof(buf))) {
    parameter.set(std::string(buf));
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
  addControllerTooltip(modPtr, parameter.getName());
  addContributionWeights(modPtr, parameter.getName());
}

void addContributionWeights(const ModPtr& modPtr, const std::string& paramName) {
  if (modPtr->sourceNameControllerPtrMap.contains(paramName)) {
    auto* controllerPtr = modPtr->sourceNameControllerPtrMap.at(paramName);
    float wAuto = controllerPtr->hasReceivedAutoValue ? controllerPtr->wAuto : 0.0f;
    float wIntent = controllerPtr->hasReceivedIntentValue ? controllerPtr->wIntent : 0.0f;
    float wManual = controllerPtr->wManual;
    float sum = wAuto + wIntent + wManual;
    if (sum > 1e-6f) {
      wAuto /= sum;
      wIntent /= sum;
      wManual /= sum;
    }
    ImGuiUtil::drawProportionalSegmentedLine(wAuto, wIntent, wManual);
  }
}

void addParameter(const ModPtr& modPtr, ofAbstractParameter& parameter) {
  if (parameter.type() == typeid(ofParameterGroup).name()) {
    if (ImGui::TreeNode(parameter.getName().c_str())) {
      addParameterGroup(modPtr, parameter.castGroup());
      ImGui::TreePop();
    }
  } else if (parameter.type() == typeid(ofParameter<int>).name()) {
    auto& intParam = parameter.cast<int>();
    addParameter(modPtr, intParam);
  } else if (parameter.type() == typeid(ofParameter<float>).name()) {
    auto& floatParam = parameter.cast<float>();
    addParameter(modPtr, floatParam);
  } else if (parameter.type() == typeid(ofParameter<bool>).name()) {
    auto& boolParam = parameter.cast<bool>();
    addParameter(modPtr, boolParam);
  } else if (parameter.type() == typeid(ofParameter<std::string>).name()) {
    auto& stringParam = parameter.cast<std::string>();
    addParameter(modPtr, stringParam);
  } else if (parameter.type() == typeid(ofParameter<ofFloatColor>).name()) {
    auto& colorParam = parameter.cast<ofFloatColor>();
    addParameter(modPtr, colorParam);
  } else if (parameter.type() == typeid(ofParameter<glm::vec2>).name()) {
    auto& vec2Param = parameter.cast<glm::vec2>();
    addParameter(modPtr, vec2Param);
  } else {
    ImGui::Text("Unsupported parameter type: %s", parameter.type().c_str());
  }
}

void addParameterGroup(const ModPtr& modPtr, ofParameterGroup& paramGroup) {
  for (auto& parameterPtr : paramGroup) {
    addParameter(modPtr, *parameterPtr);
  }
}



} // namespace NodeRenderUtil
} // namespace ofxMarkSynth
