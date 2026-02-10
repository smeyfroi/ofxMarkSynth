//  SomPaletteMod.cpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#include "SomPaletteMod.hpp"

#include "core/IntentMapping.hpp"
#include "core/Synth.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>

namespace ofxMarkSynth {

namespace {

constexpr float ASSUMED_FPS = 30.0f;

int secsToFrames(float secs) {
  return std::max(1, static_cast<int>(std::round(secs * ASSUMED_FPS)));
}

// Texture values have a range approximately -0.7 to 0.7
ofFloatPixels rgbToRG_Opponent(const ofFloatPixels& in) {
  const int w = in.getWidth();
  const int h = in.getHeight();

  ofFloatPixels out;
  out.allocate(w, h, 2);

  const float* src = in.getData();
  float* dst = out.getData();
  const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);

  // Orthonormal opponent axes:
  // e1 = ( 1, -1,  0) / sqrt(2)  -> red-green
  // e2 = ( 1,  1, -2) / sqrt(6)  -> blue-yellow
  constexpr float invSqrt2 = 0.7071067811865476f;
  constexpr float invSqrt6 = 0.4082482904638631f;

  for (size_t i = 0; i < n; ++i) {
    float R = src[3 * i + 0];
    float G = src[3 * i + 1];
    float B = src[3 * i + 2];

    // center around neutral gray to make outputs zero-mean
    float r = R - 0.5f;
    float g = G - 0.5f;
    float b = B - 0.5f;

    float u = (r - g) * invSqrt2; // red-green
    float v = (r + g - 2.0f * b) * invSqrt6; // blue-yellow

    dst[2 * i + 0] = u;
    dst[2 * i + 1] = v;
  }
  return out;
}

constexpr float OKLAB_L_WEIGHT = 2.0f;

float oklabCost(const Oklab& a, const Oklab& b) {
  float dL = (a.L - b.L) * OKLAB_L_WEIGHT;
  float da = a.a - b.a;
  float db = a.b - b.b;
  return dL * dL + da * da + db * db;
}

float oklabChromaCost(const Oklab& a, const Oklab& b) {
  float da = a.a - b.a;
  float db = a.b - b.b;
  return da * da + db * db;
}

std::array<int, SomPalette::size> solveAssignment(
  const std::array<std::array<float, SomPalette::size>, SomPalette::size>& cost) {
  constexpr int N = static_cast<int>(SomPalette::size);
  constexpr int FULL = 1 << N;

  std::array<float, FULL> dp;
  dp.fill(std::numeric_limits<float>::infinity());
  dp[0] = 0.0f;

  std::array<int, FULL> parentChoice;
  std::array<int, FULL> parentMask;
  parentChoice.fill(-1);
  parentMask.fill(-1);

  for (int mask = 0; mask < FULL; ++mask) {
    int i = __builtin_popcount(static_cast<unsigned int>(mask));
    if (i >= N) continue;
    float base = dp[static_cast<size_t>(mask)];
    if (!std::isfinite(base)) continue;

    for (int j = 0; j < N; ++j) {
      if (mask & (1 << j)) continue;
      int nextMask = mask | (1 << j);
      float nextCost = base + cost[static_cast<size_t>(i)][static_cast<size_t>(j)];
      if (nextCost < dp[static_cast<size_t>(nextMask)]) {
        dp[static_cast<size_t>(nextMask)] = nextCost;
        parentChoice[static_cast<size_t>(nextMask)] = j;
        parentMask[static_cast<size_t>(nextMask)] = mask;
      }
    }
  }

  std::array<int, SomPalette::size> assignment;
  int mask = FULL - 1;
  for (int i = N - 1; i >= 0; --i) {
    int j = parentChoice[static_cast<size_t>(mask)];
    assignment[static_cast<size_t>(i)] = j;
    mask = parentMask[static_cast<size_t>(mask)];
  }

  return assignment;
}

std::string serializeOklabList(const Oklab* labs, size_t count) {
  std::ostringstream ss;
  ss << std::setprecision(9);
  for (size_t i = 0; i < count; ++i) {
    if (i > 0) ss << ';';
    ss << labs[i].L << ',' << labs[i].a << ',' << labs[i].b;
  }
  return ss.str();
}

bool parseOklabTriple(const std::string& token, Oklab& out) {
  std::stringstream ss(token);
  std::string part;
  float v[3];
  int i = 0;

  try {
    while (i < 3 && std::getline(ss, part, ',')) {
      v[i++] = std::stof(part);
    }
  } catch (...) {
    return false;
  }

  if (i != 3) return false;
  out = { v[0], v[1], v[2] };
  return true;
}

bool parseOklabList(const std::string& s, std::vector<Oklab>& out, size_t maxCount) {
  out.clear();

  std::stringstream ss(s);
  std::string token;
  while (std::getline(ss, token, ';')) {
    if (token.empty()) continue;
    if (out.size() >= maxCount) break;

    Oklab lab;
    if (!parseOklabTriple(token, lab)) {
      return false;
    }
    out.push_back(lab);
  }

  return !out.empty();
}

} // namespace

SomPaletteMod::SomPaletteMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  noveltyCache.reserve(NOVELTY_CACHE_SIZE);
  pendingNovelty.reserve(NOVELTY_CACHE_SIZE);

  somPalette.setNumIterations(static_cast<int>(iterationsParameter.get()));

  windowFrames = secsToFrames(windowSecsParameter.get());
  somPalette.setWindowFrames(windowFrames);

  somPalette.setVisible(false);
  somPalette.setColorizerGains(colorizerGrayGainParameter, colorizerChromaGainParameter);

  sinkNameIdMap = {
    { "Sample", SINK_VEC3 },
    { "SwitchPalette", SINK_SWITCH_PALETTE }
  };
  sourceNameIdMap = {
    { "Random", SOURCE_RANDOM },
    { "RandomNovelty", SOURCE_RANDOM_NOVELTY },
    { "RandomLight", SOURCE_RANDOM_LIGHT },
    { "RandomDark", SOURCE_RANDOM_DARK },
    { "Darkest", SOURCE_DARKEST },
    { "Lightest", SOURCE_LIGHTEST },
    { "FieldTexture", SOURCE_FIELD }
  };

  for (int i = 0; i < static_cast<int>(SomPalette::size); ++i) {
    persistentIndicesByLightness[static_cast<size_t>(i)] = i;
  }
}

void SomPaletteMod::doneModLoad() {
  auto synth = getSynth();
  if (!synth) return;
  auto self = std::static_pointer_cast<SomPaletteMod>(shared_from_this());
  std::string baseName = getName();

  synth->addLiveTexturePtrFn(baseName + ": Active",
                             [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getActivePaletteTexturePtr();
    }
    return nullptr;
  });

  synth->addLiveTexturePtrFn(baseName + ": Next",
                             [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getNextPaletteTexturePtr();
    }
    return nullptr;
  });

  synth->addLiveTexturePtrFn(baseName + ": Chips",
                             [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getChipsTexturePtr();
    }
    return nullptr;
  });

}

SomPaletteMod::~SomPaletteMod() {
  iterationsParameter.removeListener(this, &SomPaletteMod::onIterationsParameterChanged);
  windowSecsParameter.removeListener(this, &SomPaletteMod::onWindowSecsParameterChanged);
  colorizerGrayGainParameter.removeListener(this, &SomPaletteMod::onColorizerParameterChanged);
  colorizerChromaGainParameter.removeListener(this, &SomPaletteMod::onColorizerParameterChanged);
}

Mod::UiState SomPaletteMod::captureUiState() const {
  Mod::UiState state;
  setUiStateBool(state, "visible", somPalette.isVisible());
  return state;
}

void SomPaletteMod::restoreUiState(const Mod::UiState& state) {
  const bool defaultVisible = somPalette.isVisible();
  somPalette.setVisible(getUiStateBool(state, "visible", defaultVisible));
}

Mod::RuntimeState SomPaletteMod::captureRuntimeState() const {
  Mod::RuntimeState state;

  if (hasPersistentChips) {
    state["persistentChipsLab"] = serializeOklabList(persistentChipsLab.data(), persistentChipsLab.size());
  }

  if (!noveltyCache.empty()) {
    std::array<Oklab, NOVELTY_CACHE_SIZE> labs;
    const size_t n = std::min<size_t>(noveltyCache.size(), NOVELTY_CACHE_SIZE);
    for (size_t i = 0; i < n; ++i) {
      labs[i] = noveltyCache[i].lab;
    }
    state["noveltyCacheLab"] = serializeOklabList(labs.data(), n);
  }

  return state;
}

void SomPaletteMod::restoreRuntimeState(const Mod::RuntimeState& state) {
  bool restoredAny = false;

  auto chipsIt = state.find("persistentChipsLab");
  if (chipsIt != state.end() && !chipsIt->second.empty()) {
    std::vector<Oklab> parsed;
    if (parseOklabList(chipsIt->second, parsed, SomPalette::size) && parsed.size() == SomPalette::size) {
      for (size_t i = 0; i < SomPalette::size; ++i) {
        persistentChipsLab[i] = parsed[i];
        persistentChipsRgb[i] = oklabToRgb(parsed[i]);
      }
      hasPersistentChips = true;
      restoredAny = true;

      for (size_t i = 0; i < SomPalette::size; ++i) {
        persistentIndicesByLightness[i] = static_cast<int>(i);
      }
      std::sort(persistentIndicesByLightness.begin(), persistentIndicesByLightness.end(), [this](int a, int b) {
        return persistentChipsLab[static_cast<size_t>(a)].L < persistentChipsLab[static_cast<size_t>(b)].L;
      });
    }
  }

  noveltyCache.clear();
  pendingNovelty.clear();

  auto noveltyIt = state.find("noveltyCacheLab");
  if (noveltyIt != state.end() && !noveltyIt->second.empty()) {
    std::vector<Oklab> parsed;
    if (parseOklabList(noveltyIt->second, parsed, NOVELTY_CACHE_SIZE)) {
      for (const auto& lab : parsed) {
        noveltyCache.push_back(CachedNovelty { lab, oklabToRgb(lab), 0.0f, paletteFrameCount });
      }
      restoredAny = true;
    }
  }

  // If we're restoring chips from a previous config, skip startup fade.
  if (restoredAny) {
    const float fadeFrames = std::max(1.0f, startupFadeSecsParameter.get() * ASSUMED_FPS);
    paletteFrameCount = static_cast<int64_t>(std::round(fadeFrames));
    firstSampleFrameCount = 0;
    startupFadeFactor = 1.0f;

    updateChipsTexture();
  }
}

const ofTexture* SomPaletteMod::getActivePaletteTexturePtr() const {
  return somPalette.getActiveTexturePtr();
}

const ofTexture* SomPaletteMod::getNextPaletteTexturePtr() const {
  return somPalette.getNextTexturePtr();
}

const ofTexture* SomPaletteMod::getChipsTexturePtr() const {
  return chipsTexture.isAllocated() ? &chipsTexture : nullptr;
}


void SomPaletteMod::onIterationsParameterChanged(float& value) {
  somPalette.setNumIterations(static_cast<int>(iterationsParameter.get()));
}

void SomPaletteMod::onWindowSecsParameterChanged(float& value) {
  windowFrames = secsToFrames(windowSecsParameter.get());
  somPalette.setWindowFrames(windowFrames);
  while (static_cast<int>(featureHistory.size()) > windowFrames) {
    featureHistory.pop_front();
  }
}

void SomPaletteMod::onColorizerParameterChanged(float& value) {
  somPalette.setColorizerGains(colorizerGrayGainParameter, colorizerChromaGainParameter);
}

void SomPaletteMod::initParameters() {
  parameters.add(iterationsParameter);
  parameters.add(windowSecsParameter);
  parameters.add(chipMemoryMultiplierParameter);
  parameters.add(startupFadeSecsParameter);
  parameters.add(trainingStepsPerFrameParameter);
  parameters.add(agencyFactorParameter);
  parameters.add(noveltyEmitChanceParameter);
  parameters.add(antiCollapseJitterParameter);
  parameters.add(antiCollapseVarianceSecsParameter);
  parameters.add(antiCollapseVarianceThresholdParameter);
  parameters.add(antiCollapseDriftSpeedParameter);
  parameters.add(colorizerGrayGainParameter);
  parameters.add(colorizerChromaGainParameter);

  iterationsParameter.addListener(this, &SomPaletteMod::onIterationsParameterChanged);
  windowSecsParameter.addListener(this, &SomPaletteMod::onWindowSecsParameterChanged);
  colorizerGrayGainParameter.addListener(this, &SomPaletteMod::onColorizerParameterChanged);
  colorizerChromaGainParameter.addListener(this, &SomPaletteMod::onColorizerParameterChanged);
}

float SomPaletteMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SomPaletteMod::ensureFieldTexture(int w, int h) {
  if (fieldTexture.isAllocated() && fieldTexture.getWidth() == w && fieldTexture.getHeight() == h) {
    return;
  }
  ofTextureData texData;
  texData.width = w;
  texData.height = h;
  texData.textureTarget = GL_TEXTURE_2D;
  texData.glInternalFormat = GL_RG16F;
  texData.bFlipTexture = false;
  texData.wrapModeHorizontal = GL_REPEAT;
  texData.wrapModeVertical = GL_REPEAT;

  fieldTexture.allocate(texData);
}

void SomPaletteMod::updateChipsTexture() {
  constexpr int w = static_cast<int>(SomPalette::size);
  constexpr int h = 2;

  if (!chipsPixels.isAllocated() || chipsPixels.getWidth() != w || chipsPixels.getHeight() != h) {
    chipsPixels.allocate(w, h, OF_IMAGE_COLOR);
  }

  // Row 0 (top): main palette chips (persistent lightness ordering if available).
  for (int x = 0; x < w; ++x) {
    ofFloatColor c;
    if (hasPersistentChips) {
      const int index = persistentIndicesByLightness[static_cast<size_t>(x)];
      c = persistentChipsRgb[static_cast<size_t>(index)];
    } else {
      c = ofFloatColor(somPalette.getColor(x));
    }

    c.r *= startupFadeFactor;
    c.g *= startupFadeFactor;
    c.b *= startupFadeFactor;

    chipsPixels.setColor(x, 0, c);
  }

  // Row 1 (bottom): novelty cache chips, repeated across the full width.
  for (int x = 0; x < w; ++x) {
    const int slot = (x * NOVELTY_CACHE_SIZE) / w;

    ofFloatColor c(0.0f, 0.0f, 0.0f);
    if (slot >= 0 && slot < static_cast<int>(noveltyCache.size())) {
      c = noveltyCache[static_cast<size_t>(slot)].rgb;
    }

    c.r *= startupFadeFactor;
    c.g *= startupFadeFactor;
    c.b *= startupFadeFactor;

    chipsPixels.setColor(x, 1, c);
  }

  const bool needsAlloc = !chipsTexture.isAllocated()
    || static_cast<int>(chipsTexture.getWidth()) != w
    || static_cast<int>(chipsTexture.getHeight()) != h;

  if (needsAlloc) {
    chipsTexture.allocate(chipsPixels, false);
    chipsTexture.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    chipsTexture.setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
  }

  chipsTexture.loadData(chipsPixels);
}


void SomPaletteMod::updateNoveltyCache(const std::vector<Oklab>& candidatesLab,
                                     const std::vector<ofFloatColor>& candidatesRgb,
                                     float dt) {
  if (!hasPersistentChips) return;

  const size_t n = std::min(candidatesLab.size(), candidatesRgb.size());
  if (n == 0) return;

  const int requiredFrames = std::max(2, static_cast<int>(std::round(0.5f * ASSUMED_FPS)));
  (void)dt;

  // We keep novelty mostly about hue/chroma (Oklab a/b), not lightness.
  const float noveltyThreshold = 0.010f; // oklabCost() (includes weighted lightness)
  const float noveltyChromaThreshold = 0.0025f; // oklabChromaCost() (a/b only)

  const float pendingMergeChromaThreshold = 0.0015f;
  const float cacheMatchChromaThreshold = 0.0012f;
  const float cacheMinSeparationChromaThreshold = 0.0035f;
  const float replaceMargin = 0.0005f;

  // Filter out near-neutral candidates; allow very dark colors only if they're still chromatic.
  const float minCandidateChroma2 = 0.0009f; // chroma ~ 0.03
  const float darkL = 0.08f;
  const float darkMinCandidateChroma2 = 0.0036f; // chroma ~ 0.06

  const float memorySecs = std::max(0.001f, windowSecsParameter.get() * chipMemoryMultiplierParameter.get());
  const int64_t ttlFrames = std::max<int64_t>(1, static_cast<int64_t>(std::round(memorySecs * ASSUMED_FPS)));
  const int64_t pendingTimeoutFrames = std::max<int64_t>(requiredFrames, static_cast<int64_t>(std::round(0.75f * ASSUMED_FPS)));

  std::vector<float> candChromaScore(n, std::numeric_limits<float>::infinity());
  std::vector<uint8_t> candEligible(n, 0);

  for (size_t j = 0; j < n; ++j) {
    const Oklab& cand = candidatesLab[j];
    const float chroma2 = cand.a * cand.a + cand.b * cand.b;
    if (chroma2 < minCandidateChroma2) continue;
    if (cand.L < darkL && chroma2 < darkMinCandidateChroma2) continue;

    float bestNovelty = std::numeric_limits<float>::infinity();
    float bestChroma = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < SomPalette::size; ++i) {
      bestNovelty = std::min(bestNovelty, oklabCost(persistentChipsLab[i], cand));
      bestChroma = std::min(bestChroma, oklabChromaCost(persistentChipsLab[i], cand));
    }

    candChromaScore[j] = bestChroma;

    if (bestNovelty >= noveltyThreshold && bestChroma >= noveltyChromaThreshold) {
      candEligible[j] = 1;
    }
  }

  // Refresh cache items only when we see genuinely-novel candidates near them.
  // This prevents cached colors from being kept alive by baseline-like colors.
  for (size_t j = 0; j < n; ++j) {
    if (!candEligible[j]) continue;

    const Oklab& cand = candidatesLab[j];
    for (auto& cached : noveltyCache) {
      float d = oklabChromaCost(cached.lab, cand);
      if (d < cacheMatchChromaThreshold) {
        cached.lastSeenFrame = paletteFrameCount;
        cached.chromaNoveltyScore = std::max(cached.chromaNoveltyScore, candChromaScore[j]);
      }
    }
  }

  // Purge expired cache entries.
  noveltyCache.erase(std::remove_if(noveltyCache.begin(), noveltyCache.end(), [&](const CachedNovelty& c) {
    return (paletteFrameCount - c.lastSeenFrame) > ttlFrames;
  }), noveltyCache.end());

  // Update cached novelty scores (distance in chroma from the main palette).
  for (auto& cached : noveltyCache) {
    float best = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < SomPalette::size; ++i) {
      best = std::min(best, oklabChromaCost(cached.lab, persistentChipsLab[i]));
    }
    cached.chromaNoveltyScore = best;
  }

  // Update pending candidates based on novelty score.
  for (size_t j = 0; j < n; ++j) {
    if (!candEligible[j]) continue;

    const Oklab candLab = candidatesLab[j];
    const ofFloatColor candRgb = candidatesRgb[j];
    const float chromaScore = candChromaScore[j];

    int bestIndex = -1;
    float bestDist = std::numeric_limits<float>::infinity();

    for (int i = 0; i < static_cast<int>(pendingNovelty.size()); ++i) {
      float d = oklabChromaCost(pendingNovelty[static_cast<size_t>(i)].lab, candLab);
      if (d < bestDist) {
        bestDist = d;
        bestIndex = i;
      }
    }

    if (bestIndex >= 0 && bestDist < pendingMergeChromaThreshold) {
      auto& p = pendingNovelty[static_cast<size_t>(bestIndex)];
      p.lab = candLab;
      p.rgb = candRgb;
      p.chromaNoveltyScore = std::max(p.chromaNoveltyScore, chromaScore);
      p.framesSeen++;
      p.lastSeenFrame = paletteFrameCount;
    } else if (static_cast<int>(pendingNovelty.size()) < NOVELTY_CACHE_SIZE) {
      bool tooClose = false;
      for (const auto& cached : noveltyCache) {
        if (oklabChromaCost(cached.lab, candLab) < cacheMinSeparationChromaThreshold) {
          tooClose = true;
          break;
        }
      }
      if (!tooClose) {
        for (const auto& pending : pendingNovelty) {
          if (oklabChromaCost(pending.lab, candLab) < cacheMinSeparationChromaThreshold) {
            tooClose = true;
            break;
          }
        }
      }

      if (!tooClose) {
        pendingNovelty.push_back(PendingNovelty { candLab, candRgb, chromaScore, 1, paletteFrameCount });
      }
    }
  }

  // Purge stale pending.
  pendingNovelty.erase(std::remove_if(pendingNovelty.begin(), pendingNovelty.end(), [&](const PendingNovelty& p) {
    return (paletteFrameCount - p.lastSeenFrame) > pendingTimeoutFrames;
  }), pendingNovelty.end());

  // Promote pending that stick around.
  for (auto it = pendingNovelty.begin(); it != pendingNovelty.end();) {
    if (it->framesSeen < requiredFrames) {
      ++it;
      continue;
    }

    // Avoid duplicates: if it matches an existing cache entry, refresh that entry instead.
    int bestCacheIndex = -1;
    float bestCacheDist = std::numeric_limits<float>::infinity();
    for (int i = 0; i < static_cast<int>(noveltyCache.size()); ++i) {
      float d = oklabChromaCost(noveltyCache[static_cast<size_t>(i)].lab, it->lab);
      if (d < bestCacheDist) {
        bestCacheDist = d;
        bestCacheIndex = i;
      }
    }

    if (bestCacheIndex >= 0 && bestCacheDist < cacheMatchChromaThreshold) {
      // Only update the cached color if the incoming one is meaningfully more novel.
      // Otherwise, keep the existing (more interesting) cached color and just extend its life.
      auto& c = noveltyCache[static_cast<size_t>(bestCacheIndex)];
      if (it->chromaNoveltyScore > c.chromaNoveltyScore + replaceMargin) {
        c.lab = it->lab;
        c.rgb = it->rgb;
      }
      c.chromaNoveltyScore = std::max(c.chromaNoveltyScore, it->chromaNoveltyScore);
      c.lastSeenFrame = paletteFrameCount;
      it = pendingNovelty.erase(it);
      continue;
    }

    bool tooClose = false;
    for (const auto& c : noveltyCache) {
      if (oklabChromaCost(c.lab, it->lab) < cacheMinSeparationChromaThreshold) {
        tooClose = true;
        break;
      }
    }
    if (tooClose) {
      it = pendingNovelty.erase(it);
      continue;
    }

    if (static_cast<int>(noveltyCache.size()) < NOVELTY_CACHE_SIZE) {
      noveltyCache.push_back(CachedNovelty { it->lab, it->rgb, it->chromaNoveltyScore, paletteFrameCount });
    } else {
      // Replace the least-novel cached entry, but only if we're meaningfully more novel.
      int worstIndex = 0;
      float worstScore = noveltyCache[0].chromaNoveltyScore;
      int64_t worstSeen = noveltyCache[0].lastSeenFrame;

      for (int i = 1; i < static_cast<int>(noveltyCache.size()); ++i) {
        const auto& c = noveltyCache[static_cast<size_t>(i)];
        if (c.chromaNoveltyScore < worstScore || (c.chromaNoveltyScore == worstScore && c.lastSeenFrame < worstSeen)) {
          worstIndex = i;
          worstScore = c.chromaNoveltyScore;
          worstSeen = c.lastSeenFrame;
        }
      }

      if (it->chromaNoveltyScore > worstScore + replaceMargin) {
        noveltyCache[static_cast<size_t>(worstIndex)] = CachedNovelty { it->lab, it->rgb, it->chromaNoveltyScore, paletteFrameCount };
      }
    }

    it = pendingNovelty.erase(it);
  }
}


void SomPaletteMod::updatePersistentChips(float dt) {
  constexpr size_t N = SomPalette::size;

  std::array<ofFloatColor, N> candidatesRgb;
  std::array<Oklab, N> candidatesLab;

  for (size_t i = 0; i < N; ++i) {
    ofFloatColor c = ofFloatColor(somPalette.getColor(static_cast<int>(i)));
    candidatesRgb[i] = c;
    candidatesLab[i] = rgbToOklab(c);
  }

  if (!hasPersistentChips) {
    persistentChipsLab = candidatesLab;
    persistentChipsRgb = candidatesRgb;
    hasPersistentChips = true;
  } else {
    std::array<std::array<float, N>, N> cost;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        cost[i][j] = oklabCost(persistentChipsLab[i], candidatesLab[j]);
      }
    }

    const auto assignment = solveAssignment(cost);

    const float memorySecs = std::max(0.001f, windowSecsParameter.get() * chipMemoryMultiplierParameter.get());
    const float alpha = 1.0f - std::exp(-dt / memorySecs);

    for (size_t i = 0; i < N; ++i) {
      const size_t j = static_cast<size_t>(assignment[i]);
      const Oklab target = candidatesLab[j];

      persistentChipsLab[i].L += alpha * (target.L - persistentChipsLab[i].L);
      persistentChipsLab[i].a += alpha * (target.a - persistentChipsLab[i].a);
      persistentChipsLab[i].b += alpha * (target.b - persistentChipsLab[i].b);

      persistentChipsRgb[i] = oklabToRgb(persistentChipsLab[i]);
    }
  }

  for (size_t i = 0; i < N; ++i) {
    persistentIndicesByLightness[i] = static_cast<int>(i);
  }
  std::sort(persistentIndicesByLightness.begin(), persistentIndicesByLightness.end(), [this](int a, int b) {
    return persistentChipsLab[static_cast<size_t>(a)].L < persistentChipsLab[static_cast<size_t>(b)].L;
  });

}

int SomPaletteMod::getPersistentDarkestIndex() const {
  if (!hasPersistentChips) return 0;
  return persistentIndicesByLightness.front();
}

int SomPaletteMod::getPersistentLightestIndex() const {
  if (!hasPersistentChips) return static_cast<int>(SomPalette::size - 1);
  return persistentIndicesByLightness.back();
}

glm::vec3 SomPaletteMod::computeRecentFeatureVarianceVec(int frames) const {
  const int n = std::min(frames, static_cast<int>(featureHistory.size()));
  if (n < 2) return glm::vec3 { 0.0f };

  auto it = featureHistory.end();
  std::advance(it, -n);

  glm::vec3 mean { 0.0f };
  for (auto p = it; p != featureHistory.end(); ++p) {
    mean += *p;
  }
  mean /= static_cast<float>(n);

  glm::vec3 var { 0.0f };
  for (auto p = it; p != featureHistory.end(); ++p) {
    const glm::vec3 d = *p - mean;
    var += d * d;
  }
  var /= static_cast<float>(n - 1);
  return var;
}

float SomPaletteMod::computeRecentFeatureVariance(int frames) const {
  const glm::vec3 var = computeRecentFeatureVarianceVec(frames);
  return (var.x + var.y + var.z) / 3.0f;
}

float SomPaletteMod::computeAntiCollapseFactor(const glm::vec3& varianceVec) const {
  const float jitter = antiCollapseJitterParameter.get();
  if (jitter <= 0.0f) return 0.0f;

  const float threshold = std::max(1.0e-8f, antiCollapseVarianceThresholdParameter.get());
  const float variance = (varianceVec.x + varianceVec.y + varianceVec.z) / 3.0f;

  const float shortfall = ofClamp((threshold - variance) / threshold, 0.0f, 1.0f);

  // Be more assertive near the threshold (sqrt curve).
  return std::sqrt(shortfall);
}

glm::vec3 SomPaletteMod::applyAntiCollapseJitter(const glm::vec3& v,
                                                float factor,
                                                const glm::vec3& varianceVec,
                                                float timeSecs,
                                                int step) const {
  if (factor <= 0.0f) return v;

  // Scale per-component jitter by how much that feature's variance falls short of the target.
  // This keeps the injected variation local to the *audio feature space* rather than picking unrelated colors.
  const float threshold = std::max(1.0e-8f, antiCollapseVarianceThresholdParameter.get());
  const glm::vec3 stdTarget { std::sqrt(threshold) };
  const glm::vec3 stdNow { std::sqrt(std::max(0.0f, varianceVec.x)),
                           std::sqrt(std::max(0.0f, varianceVec.y)),
                           std::sqrt(std::max(0.0f, varianceVec.z)) };
  const glm::vec3 stdDeficit = glm::max(glm::vec3 { 0.0f }, stdTarget - stdNow);
  const glm::vec3 deficitFrac = stdDeficit / glm::max(glm::vec3 { 1.0e-8f }, stdTarget);

  const float baseAmp = antiCollapseJitterParameter.get() * factor;
  const glm::vec3 amp = baseAmp * deficitFrac;

  const float speed = antiCollapseDriftSpeedParameter.get();

  // Smooth noise, with phase partially keyed from the current feature so sustained tones
  // remain coherent and changes in timbre/register shift the jitter field.
  const float phase = timeSecs * speed + 17.0f * v.x + 29.0f * v.y + 43.0f * v.z + static_cast<float>(step) * 0.173f;
  const glm::vec3 n {
    ofSignedNoise(phase, 11.1f),
    ofSignedNoise(phase, 22.2f),
    ofSignedNoise(phase, 33.3f),
  };

  return glm::clamp(v + amp * n, glm::vec3 { 0.0f }, glm::vec3 { 1.0f });
}

void SomPaletteMod::update() {
  ++paletteFrameCount;

  syncControllerAgencies();

  // Expect one incoming feature per frame, but be robust to bursts.
  bool hasNewFeature = !newVecs.empty();
  glm::vec3 newestFeature;
  if (hasNewFeature) {
    newestFeature = newVecs.back();
    newVecs.clear();

    if (firstSampleFrameCount < 0) {
      firstSampleFrameCount = paletteFrameCount;
    }

    featureHistory.push_back(newestFeature);
    while (static_cast<int>(featureHistory.size()) > windowFrames) {
      featureHistory.pop_front();
    }
  }

  // Train multiple steps per frame by sampling from the recent history.
  const int steps = std::max(1, trainingStepsPerFrameParameter.get());
  if (!featureHistory.empty()) {
    std::uniform_int_distribution<size_t> dist(0, featureHistory.size() - 1);

    const float secs = std::max(0.01f, antiCollapseVarianceSecsParameter.get());
    const int frames = secsToFrames(secs);
    const glm::vec3 varianceVec = computeRecentFeatureVarianceVec(frames);
    const float antiCollapseFactor = computeAntiCollapseFactor(varianceVec);
    const float timeSecs = static_cast<float>(paletteFrameCount) / ASSUMED_FPS;

    for (int i = 0; i < steps; ++i) {
      glm::vec3 v = (hasNewFeature && i == 0) ? newestFeature : featureHistory[dist(randomGen)];
      v = applyAntiCollapseJitter(v, antiCollapseFactor, varianceVec, timeSecs, i);
      std::array<double, 3> data { v.x, v.y, v.z };
      somPalette.addInstanceData(data);
    }
  }

  somPalette.update();

  const float dt = 1.0f / ASSUMED_FPS;

  // Fade in from the first received sample.
  if (firstSampleFrameCount < 0 || startupFadeSecsParameter.get() <= 0.0f) {
    startupFadeFactor = (firstSampleFrameCount < 0) ? 0.0f : 1.0f;
  } else {
    const float fadeFrames = std::max(1.0f, startupFadeSecsParameter.get() * ASSUMED_FPS);
    startupFadeFactor = ofClamp(static_cast<float>(paletteFrameCount - firstSampleFrameCount) / fadeFrames, 0.0f, 1.0f);
  }

  // Update long-memory chip set (Oklab) from the current palette chips.
  const ofFloatPixels& pixelsRef = somPalette.getPixelsRef();
  if (pixelsRef.getWidth() > 0 && pixelsRef.getHeight() > 0) {
    updatePersistentChips(dt);

    // Novelty candidates come from the whole SOM field (audio-derived outliers),
    // not just the current 8-chip palette.
    const int w = pixelsRef.getWidth();
    const int h = pixelsRef.getHeight();

    std::vector<Oklab> noveltyCandidatesLab;
    std::vector<ofFloatColor> noveltyCandidatesRgb;
    noveltyCandidatesLab.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));
    noveltyCandidatesRgb.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const ofFloatColor c = pixelsRef.getColor(x, y);
        noveltyCandidatesRgb.push_back(c);
        noveltyCandidatesLab.push_back(rgbToOklab(c));
      }
    }

    updateNoveltyCache(noveltyCandidatesLab, noveltyCandidatesRgb, dt);
  }

  emit(SOURCE_RANDOM, createRandomVec4());
  emit(SOURCE_RANDOM_NOVELTY, createRandomNoveltyVec4());
  emit(SOURCE_RANDOM_LIGHT, createRandomLightVec4());
  emit(SOURCE_RANDOM_DARK, createRandomDarkVec4());
  emit(SOURCE_DARKEST, createVec4(getPersistentDarkestIndex()));
  emit(SOURCE_LIGHTEST, createVec4(getPersistentLightestIndex()));

  updateChipsTexture();

  if (pixelsRef.getWidth() == 0 || pixelsRef.getHeight() == 0) return;

  // convert RGB -> RG (float2), upload into float RG texture, emit texture
  ofFloatPixels converted = rgbToRG_Opponent(pixelsRef);
  ensureFieldTexture(converted.getWidth(), converted.getHeight());
  fieldTexture.loadData(converted);
  emit(SOURCE_FIELD, fieldTexture);
}

glm::vec4 SomPaletteMod::createVec4(int i) {
  const int clamped = ofClamp(i, 0, static_cast<int>(SomPalette::size - 1));
  ofFloatColor c;

  if (hasPersistentChips) {
    c = persistentChipsRgb[static_cast<size_t>(clamped)];
  } else {
    c = ofFloatColor(somPalette.getColor(clamped));
  }

  const float f = startupFadeFactor;
  return { c.r * f, c.g * f, c.b * f, f };
}

glm::vec4 SomPaletteMod::createRandomVec4() {
  // Only "catch" novelty colors; don't bias the system to chase them.
  // Once cached, we can occasionally emit them.
  const float noveltyEmitChance = ofClamp(noveltyEmitChanceParameter.get(), 0.0f, 1.0f);

  if (!noveltyCache.empty() && random01Distrib(randomGen) < noveltyEmitChance) {
    std::uniform_int_distribution<> dist(0, static_cast<int>(noveltyCache.size() - 1));
    const auto& c = noveltyCache[static_cast<size_t>(dist(randomGen))].rgb;
    const float f = startupFadeFactor;
    return { c.r * f, c.g * f, c.b * f, f };
  }

  return createVec4(randomDistrib(randomGen));
}

glm::vec4 SomPaletteMod::createRandomNoveltyVec4() {
  if (noveltyCache.empty()) {
    // Fallback behavior requested: behave like SOURCE_RANDOM when the novelty cache is empty.
    return createRandomVec4();
  }

  std::uniform_int_distribution<> dist(0, static_cast<int>(noveltyCache.size() - 1));
  const auto& c = noveltyCache[static_cast<size_t>(dist(randomGen))].rgb;
  const float f = startupFadeFactor;
  return { c.r * f, c.g * f, c.b * f, f };
}

glm::vec4 SomPaletteMod::createRandomLightVec4() {
  if (!hasPersistentChips) {
    return createVec4((SomPalette::size - 1) - randomDistrib(randomGen) / 2);
  }

  // Occasionally emit a cached novelty color that falls in the light half.
  const float noveltyEmitChance = ofClamp(noveltyEmitChanceParameter.get(), 0.0f, 1.0f);
  if (!noveltyCache.empty() && random01Distrib(randomGen) < noveltyEmitChance) {
    const int lo = persistentIndicesByLightness[SomPalette::size / 2 - 1];
    const int hi = persistentIndicesByLightness[SomPalette::size / 2];
    const float midL = 0.5f
      * (persistentChipsLab[static_cast<size_t>(lo)].L + persistentChipsLab[static_cast<size_t>(hi)].L);

    int eligible[NOVELTY_CACHE_SIZE];
    int count = 0;
    for (int i = 0; i < static_cast<int>(noveltyCache.size()) && i < NOVELTY_CACHE_SIZE; ++i) {
      if (noveltyCache[static_cast<size_t>(i)].lab.L >= midL) {
        eligible[count++] = i;
      }
    }

    if (count > 0) {
      std::uniform_int_distribution<> dist(0, count - 1);
      const auto& c = noveltyCache[static_cast<size_t>(eligible[dist(randomGen)])].rgb;
      const float f = startupFadeFactor;
      return { c.r * f, c.g * f, c.b * f, f };
    }
  }

  const int offset = static_cast<int>(SomPalette::size / 2);
  const int slot = offset + randomHalfDistrib(randomGen);
  const int index = persistentIndicesByLightness[static_cast<size_t>(slot)];
  return createVec4(index);
}

glm::vec4 SomPaletteMod::createRandomDarkVec4() {
  if (!hasPersistentChips) {
    return createVec4(randomDistrib(randomGen) / 2);
  }

  // Occasionally emit a cached novelty color that falls in the dark half.
  const float noveltyEmitChance = ofClamp(noveltyEmitChanceParameter.get(), 0.0f, 1.0f);
  if (!noveltyCache.empty() && random01Distrib(randomGen) < noveltyEmitChance) {
    const int lo = persistentIndicesByLightness[SomPalette::size / 2 - 1];
    const int hi = persistentIndicesByLightness[SomPalette::size / 2];
    const float midL = 0.5f
      * (persistentChipsLab[static_cast<size_t>(lo)].L + persistentChipsLab[static_cast<size_t>(hi)].L);

    int eligible[NOVELTY_CACHE_SIZE];
    int count = 0;
    for (int i = 0; i < static_cast<int>(noveltyCache.size()) && i < NOVELTY_CACHE_SIZE; ++i) {
      if (noveltyCache[static_cast<size_t>(i)].lab.L < midL) {
        eligible[count++] = i;
      }
    }

    if (count > 0) {
      std::uniform_int_distribution<> dist(0, count - 1);
      const auto& c = noveltyCache[static_cast<size_t>(eligible[dist(randomGen)])].rgb;
      const float f = startupFadeFactor;
      return { c.r * f, c.g * f, c.b * f, f };
    }
  }

  const int slot = randomHalfDistrib(randomGen);
  const int index = persistentIndicesByLightness[static_cast<size_t>(slot)];
  return createVec4(index);
}

void SomPaletteMod::draw() {
  somPalette.draw();
}

bool SomPaletteMod::keyPressed(int key) {
  return somPalette.keyPressed(key);
}

void SomPaletteMod::receive(int sinkId, const glm::vec3& v) {
  switch (sinkId) {
    case SINK_VEC3:
      newVecs.push_back(v);
      break;
    default:
      ofLogError("SomPaletteMod") << "glm::vec3 receive for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_SWITCH_PALETTE:
      if (somPalette.nextPaletteIsReady() && v > 0.5f) {
        ofLogNotice("SomPaletteMod") << "SomPaletteMod switching palette";
        somPalette.switchPalette();
      }
      break;
    default:
      ofLogError("SomPaletteMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::applyIntent(const Intent& intent, float strength) {
  // no-op for now
}

} // namespace ofxMarkSynth
