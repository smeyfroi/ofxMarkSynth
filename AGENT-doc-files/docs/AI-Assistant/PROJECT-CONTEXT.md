MarkSynth is an openframeworks addon that models a computational drawing
as a set of modules that connect to each other, responding to audio
and video streams and creating live visuals for projection. The visuals
are created at very high resolution so that they can be saved as EXR
images to make physical work from.

The primary target for MarkSynth is MacOS, which limits the version of
OpenGL to 4.1. This excludes Compute Shaders, which are therefore
not used.

A Mod is a self-contained module that has sinks and slots to emit and
receive data, including floats, glm::vec2 and ofTexture references.

MarkSynth depends on a number of addons including ofxParticleField,
ofxDividedArea and others. These addons are in the openframeworks
`addons/` directory. Addons that a Mod wraps are considered part of
the MarkSynth codebase and may be edited while implementing functionality
in MarkSynth itself.

Each Mod exposes its configuration as ofParameter sets that drive
the application GUI. These parameters are used to manually configure
characteristics of each Mod, but the a ParamController wraps the
parameters so it can blend the automatic response as well as
manually-controlled Intents, which wrap a set of Module ofParameters
under a descriptor like "Calm" or "Chaotic".

High-level control over the parameters is exercised using a MIDI
control surface in live performance, but a GUI also presents all the
detailed module parameters.

MarkSynth is used in realtime performance situations so it offloads
work onto the GPU using OpenGL, as well as using CPU threads for
async processing. Performance is a critical characteristic of the
project.
