//
//  ofxMarkSynth.h
//  ofxMarkSynth
//
//  Main include file for the ofxMarkSynth addon.
//  Include this single header to access all public APIs.
//

#pragma once

// Core framework
#include "core/Synth.hpp"
#include "core/Mod.hpp"
#include "core/Intent.hpp"
#include "core/IntentMapper.hpp"
#include "core/IntentMapping.hpp"
#include "core/ParamController.h"

// Source Mods - generate data/signals
#include "sourceMods/AudioDataSourceMod.hpp"
#include "sourceMods/RandomFloatSourceMod.hpp"
#include "sourceMods/RandomHslColorMod.hpp"
#include "sourceMods/RandomVecSourceMod.hpp"
#include "sourceMods/StaticTextSourceMod.hpp"
#include "sourceMods/TextSourceMod.hpp"
#include "sourceMods/TimerSourceMod.hpp"
#include "sourceMods/VideoFlowSourceMod.hpp"

// Process Mods - transform data
#include "processMods/ClusterMod.hpp"
#include "processMods/MultiplyAddMod.hpp"
#include "processMods/PathMod.hpp"
#include "processMods/PixelSnapshotMod.hpp"
#include "processMods/SomPaletteMod.hpp"
#include "processMods/VectorMagnitudeMod.hpp"

// Layer Mods - full-layer effects
#include "layerMods/FadeMod.hpp"
#include "layerMods/FluidMod.hpp"
#include "layerMods/SmearMod.hpp"

// Sink Mods - render to layers
#include "sinkMods/CollageMod.hpp"
#include "sinkMods/DividedAreaMod.hpp"
#include "sinkMods/FluidRadialImpulseMod.hpp"
#include "sinkMods/IntrospectorMod.hpp"
#include "sinkMods/ParticleFieldMod.hpp"
#include "sinkMods/ParticleSetMod.hpp"
#include "sinkMods/SandLineMod.hpp"
#include "sinkMods/SoftCircleMod.hpp"
#include "sinkMods/TextMod.hpp"

// Config & utilities
#include "config/ModFactory.hpp"
#include "core/FontStash2Cache.hpp"
