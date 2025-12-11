//
//  HelpContent.hpp
//  ofxMarkSynth
//
//  Static help text for keyboard shortcuts window
//

#pragma once

namespace ofxMarkSynth {

constexpr const char* HELP_CONTENT = R"(GENERAL
Space           Pause / Resume
H               Toggle hibernation (fade out)
1-8             Toggle pause for layers 1-8

RECORDING & SAVING
S               Save HDR image
R               Toggle video recording

PERFORMANCE
<- (hold)       Previous config
-> (hold)       Next config

SNAPSHOTS
F1 - F8         Load snapshot from slot 1-8
Ctrl+Z          Undo last snapshot load

MOD VISIBILITY
A               PathMod
X               PixelSnapshotMod
T               Audio tuning display
C               SOM Palette
i               Introspector
V               Video (motion source)
M               Motion visualization

AUDIO PLOTS
p               Toggle scalar plots
P               Toggle spectrum/MFCC plots
<  >            Cycle analysis scalar on hovered plot

AUDIO PLAYBACK (file mode)
`               Mute / Unmute
Up              Skip forward 10s (Shift: 60s)
Down            Skip backward 10s (Shift: 60s)

SOM PALETTE
U               Save palette snapshot to ~/Documents/som/
)";

} // namespace ofxMarkSynth
