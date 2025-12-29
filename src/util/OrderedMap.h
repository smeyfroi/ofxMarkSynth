//
//  OrderedMap.h
//  ofxMarkSynth
//
//  A map container that preserves insertion order.
//  Wraps nlohmann::ordered_map to isolate the dependency.
//

#pragma once

#include "nlohmann/json.hpp"

namespace ofxMarkSynth {

/// A map container that preserves insertion order while providing key-based lookup.
/// Note: Lookup is O(n) - suitable for small collections (< 100 items).
template <typename Key, typename Value>
using OrderedMap = nlohmann::ordered_map<Key, Value>;

} // namespace ofxMarkSynth
