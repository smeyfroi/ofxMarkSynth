//  NodeRenderUtil.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 12/11/2025.
//

#include "nodeEditor/NodeRenderUtil.hpp"
#include "gui/ImGuiUtil.hpp"
#include <cmath>
#include <cstdio>



namespace ofxMarkSynth {
namespace NodeRenderUtil {

// Static mono font for tooltips
static ImFont* monoFont = nullptr;
// Optional external tooltip map for sliders (e.g. layer descriptions)
static const std::unordered_map<std::string, std::string>* externalTooltipMap = nullptr;
// Track if any parameter was modified via GUI this frame
static bool parameterModifiedThisFrame = false;

void resetModifiedFlag() {
  parameterModifiedThisFrame = false;
}

bool wasAnyParameterModified() {
  return parameterModifiedThisFrame;
}

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
  const ImVec2 sliderSize(24, 124);
  const float colW = sliderSize.x + 8.0f;   // column width (slider + padding)

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

      // Optional run toggle directly under the slider (checked = running)
      if (toggleParam) {
        float checkSize = ImGui::GetFrameHeight();
        float xPadCheck = (colW - checkSize) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPad + xPadCheck);
        bool isRunning = !toggleParam->get();  // underlying param is 'paused'
        if (ImGui::Checkbox(("##run_" + name).c_str(), &isRunning)) {
          toggleParam->set(!isRunning);
        }
        ImGui::SetItemTooltip("Enable/disable layer %s", name.c_str());
      }

      ImGui::EndGroup();

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

constexpr float sliderWidth = 200.0f;

static bool isParameterAtDefault(const ModPtr& modPtr, const std::string& fullName, const std::string& currentString) {
  const auto& defaults = modPtr->getDefaultParameterValues();
  auto it = defaults.find(fullName);
  if (it == defaults.end()) return false;
  return it->second == currentString;
}

// Helper to add tooltip showing component breakdown and final value for controlled parameters
static void addControllerTooltip(const ModPtr& modPtr, const std::string& paramName) {
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

// Helper to finish a parameter row: adds label, controller tooltip, and contribution weights
static void finishParameterRow(const ModPtr& modPtr,
                               const std::string& displayName,
                               const std::string& fullName,
                               const std::string& currentString) {
  ImGui::SameLine();

  // Make "default" (unchanged) parameters more visually distinct.
  // Use theme-aware dimming so the label stays readable.
  if (isParameterAtDefault(modPtr, fullName, currentString)) {
    constexpr float kDimRgb = 0.70f;
    constexpr float kDimAlpha = 0.85f;

    ImVec4 c = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    c.x *= kDimRgb;
    c.y *= kDimRgb;
    c.z *= kDimRgb;
    c.w *= kDimAlpha;

    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::Text("%s", displayName.c_str());
    ImGui::PopStyleColor();
  } else {
    ImGui::Text("%s", displayName.c_str());
  }

  // Note: controllers are keyed on the leaf parameter name (not full path).
  addControllerTooltip(modPtr, displayName);
  addContributionWeights(modPtr, displayName);
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<int>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  int value = parameter.get();

  std::string id = "##" + fullName;
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderInt(id.c_str(), &value, parameter.getMin(), parameter.getMax())) {
    parameter.set(value);
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  ImGui::PopItemWidth();
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<float>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  float value = parameter.get();

  std::string id = "##" + fullName;
  ImGui::PushItemWidth(sliderWidth);

  // Default formatting is based on range, but for small nonzero values we want
  // higher precision so parameters don't appear as "0.0" / "0.00".
  const float range = parameter.getMax() - parameter.getMin();
  const char* fmtRange = (range <= 0.01f) ? "%.5f" : (range <= 0.1f) ? "%.4f" : (range <= 1.0f) ? "%.3f" : "%.2f";

  const float absV = std::abs(value);
  const char* fmt = fmtRange;
  if (absV > 0.0f && absV < 1.0e-4f) {
    fmt = "%.2e";
  } else if (absV > 0.0f && absV < 1.0e-2f) {
    fmt = "%.5f";
  }

  if (ImGui::SliderFloat(id.c_str(), &value, parameter.getMin(), parameter.getMax(), fmt, ImGuiSliderFlags_NoRoundToFormat)) {
    parameter.set(value);
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  ImGui::PopItemWidth();
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<ofFloatColor>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  ofFloatColor color = parameter.get();
  float colorArray[4] = { color.r, color.g, color.b, color.a };

  std::string id = "##" + fullName;
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::ColorEdit4(id.c_str(), colorArray, ImGuiColorEditFlags_Float)) {
    parameter.set(ofFloatColor(colorArray[0], colorArray[1], colorArray[2], colorArray[3]));
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  ImGui::PopItemWidth();
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<glm::vec2>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  glm::vec2 value = parameter.get();
  float valueArray[2] = { value.x, value.y };

  std::string id = "##" + fullName;
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderFloat2(id.c_str(), valueArray, parameter.getMin().x, parameter.getMax().x, "%.2f", ImGuiSliderFlags_NoRoundToFormat)) {
    parameter.set(glm::vec2(valueArray[0], valueArray[1]));
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  ImGui::PopItemWidth();
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<bool>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  bool value = parameter.get();

  std::string id = "##" + fullName;
  if (ImGui::Checkbox(id.c_str(), &value)) {
    parameter.set(value);
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameter<std::string>& parameter, const std::string& fullName) {
  const auto& displayName = parameter.getName();
  const auto current = parameter.get();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s", current.c_str());

  std::string id = "##" + fullName;
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::InputText(id.c_str(), buf, sizeof(buf))) {
    parameter.set(std::string(buf));
    parameterModifiedThisFrame = true;
  }
  ImGui::SetItemTooltip("%s", displayName.c_str());
  ImGui::PopItemWidth();
  finishParameterRow(modPtr, displayName, fullName, parameter.toString());
}

static void addParameterInternal(const ModPtr& modPtr, ofParameterGroup& paramGroup, const std::string& prefix);
static void addParameterInternal(const ModPtr& modPtr, ofAbstractParameter& parameter, const std::string& prefix);

static void addParameterInternal(const ModPtr& modPtr, ofParameterGroup& paramGroup, const std::string& prefix) {
  for (auto& parameterPtr : paramGroup) {
    addParameterInternal(modPtr, *parameterPtr, prefix);
  }
}

static void addParameterInternal(const ModPtr& modPtr, ofAbstractParameter& parameter, const std::string& prefix) {
  const std::string displayName = parameter.getName();
  const std::string fullName = prefix.empty() ? displayName : prefix + "." + displayName;

  if (parameter.type() == typeid(ofParameterGroup).name()) {
    std::string treeLabel = displayName + "##" + fullName;
    if (ImGui::TreeNode(treeLabel.c_str())) {
      addParameterInternal(modPtr, parameter.castGroup(), fullName);
      ImGui::TreePop();
    }
  } else if (parameter.type() == typeid(ofParameter<int>).name()) {
    addParameterInternal(modPtr, parameter.cast<int>(), fullName);
  } else if (parameter.type() == typeid(ofParameter<float>).name()) {
    addParameterInternal(modPtr, parameter.cast<float>(), fullName);
  } else if (parameter.type() == typeid(ofParameter<bool>).name()) {
    addParameterInternal(modPtr, parameter.cast<bool>(), fullName);
  } else if (parameter.type() == typeid(ofParameter<std::string>).name()) {
    addParameterInternal(modPtr, parameter.cast<std::string>(), fullName);
  } else if (parameter.type() == typeid(ofParameter<ofFloatColor>).name()) {
    addParameterInternal(modPtr, parameter.cast<ofFloatColor>(), fullName);
  } else if (parameter.type() == typeid(ofParameter<glm::vec2>).name()) {
    addParameterInternal(modPtr, parameter.cast<glm::vec2>(), fullName);
  } else {
    ImGui::Text("Unsupported parameter type: %s", parameter.type().c_str());
  }
}

void addParameter(const ModPtr& modPtr, ofParameter<int>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<float>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<ofFloatColor>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<glm::vec2>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<bool>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofParameter<std::string>& parameter) {
  addParameterInternal(modPtr, parameter, parameter.getName());
}

void addParameter(const ModPtr& modPtr, ofAbstractParameter& parameter) {
  addParameterInternal(modPtr, parameter, "");
}

void addParameterGroup(const ModPtr& modPtr, ofParameterGroup& paramGroup) {
  addParameterInternal(modPtr, paramGroup, "");
}



} // namespace NodeRenderUtil
} // namespace ofxMarkSynth
