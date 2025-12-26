#pragma once

#include "Mod.hpp"
#include "Intent.hpp"
#include "ParamController.h"
#include "IntentMapping.hpp"

#include "sourceMods/StaticTextSourceMod.hpp"
#include "sourceMods/TextSourceMod.hpp"
#include "sourceMods/TimerSourceMod.hpp"
#include "sourceMods/RandomVecSourceMod.hpp"
#include "sourceMods/RandomHslColorMod.hpp"
#include "sourceMods/RandomFloatSourceMod.hpp"
#include "sourceMods/AudioDataSourceMod.hpp"
#include "sourceMods/VideoFlowSourceMod.hpp"

#include "processMods/SomPaletteMod.hpp"
#include "processMods/ClusterMod.hpp"
#include "processMods/PixelSnapshotMod.hpp"
#include "processMods/PathMod.hpp"
#include "processMods/VaryingValueMod.hpp"
#include "processMods/MultiplyAddMod.hpp"

#include "shaderMods/TranslateMod.hpp"
#include "shaderMods/AddTextureMod.hpp"

#include "layerMods/SmearMod.hpp"
#include "layerMods/FluidMod.hpp"
#include "layerMods/FadeMod.hpp"

#include "sinkMods/IntrospectorMod.hpp"
#include "sinkMods/ParticleSetMod.hpp"
#include "sinkMods/DividedAreaMod.hpp"
#include "sinkMods/SandLineMod.hpp"
#include "sinkMods/TextMod.hpp"
#include "sinkMods/CollageMod.hpp"
#include "sinkMods/FluidRadialImpulseMod.hpp"
#include "sinkMods/SoftCircleMod.hpp"
#include "sinkMods/ParticleFieldMod.hpp"

#include "util/ModFactory.hpp"
#include "util/FontCache.hpp"
#include "Synth.hpp"
