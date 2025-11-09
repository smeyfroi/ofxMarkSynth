//
//  ImGui.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 09/11/2025.
//

#include "imgui.h"



namespace ofxMarkSynth {



namespace ImGuiUtil {



constexpr ImU32 COLOR_SEGMENT1 = IM_COL32(255, 151, 151, 255);
constexpr ImU32 COLOR_SEGMENT2 = IM_COL32(51, 255, 151, 255);
constexpr ImU32 COLOR_SEGMENT3 = IM_COL32(151, 151, 255, 255);

void drawProportionalSegmentedLine(float param1, float param2, float param3) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Line parameters
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    float totalLineLength = 200.0f;  // Total available length
    float thickness = 1.0f;

    // Calculate total of all parameters
    float total = param1 + param2 + param3;

    // Handle edge case where all params are zero
    if (total < 0.001f) {
        total = 0.001f;  // Avoid division by zero
    }

    // Calculate proportional lengths for each segment
    float length1 = (param1 / total) * totalLineLength;
    float length2 = (param2 / total) * totalLineLength;
    float length3 = (param3 / total) * totalLineLength;

    // Calculate segment endpoints (cumulative positions)
    ImVec2 p1 = ImVec2(startPos.x, startPos.y);
    ImVec2 p2 = ImVec2(startPos.x + length1, startPos.y);
    ImVec2 p3 = ImVec2(startPos.x + length1 + length2, startPos.y);
    ImVec2 p4 = ImVec2(startPos.x + length1 + length2 + length3, startPos.y);

  // Draw the three segments only if they have non-zero length
  if (length1 > 0.5f) {  // Minimum visible length threshold
      drawList->AddLine(p1, p2, COLOR_SEGMENT1, thickness);
  }
  if (length2 > 0.5f) {
      drawList->AddLine(p2, p3, COLOR_SEGMENT2, thickness);
  }
  if (length3 > 0.5f) {
      drawList->AddLine(p3, p4, COLOR_SEGMENT3, thickness);
  }

  // Advance cursor for next elements
  ImGui::Dummy(ImVec2(totalLineLength, thickness + 5.0f));
}



} // namespace ImGui



} // namespace ofxMarkSynth
