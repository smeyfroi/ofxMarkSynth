//
//  FboCopy.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 13/09/2025.
//

#pragma once


#include "ofMain.h"


namespace ofxMarkSynth {


// - GPU-only; draws each color attachment from src into dst.
// - Allocates/resizes dst as needed.
void fboCopyDraw(const ofFbo& src, ofFbo& dst);

// OpenGL blit-copy.
// - Fast, exact 1:1 copy; can also copy depth if requested.
// - Allocates/resizes dst as needed.
// - Works on macOS OpenGL core profile (up to 4.1).
void fboCopyBlit(const ofFbo& src, ofFbo& dst, bool copyDepth = false);


}
