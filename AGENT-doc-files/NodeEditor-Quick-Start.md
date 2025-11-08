# Node Editor Animation - Quick Start

## What Was Changed

### Files Modified

1. **`src/nodeEditor/NodeEditorLayout.hpp/cpp`**
   - Changed `initialize()` to take `const Synth*` directly
   - Removed intermediate `LayoutLink` structs
   - Discovers connections automatically from `Mod::connections`
   - Made `isStable()` public for animation tracking

2. **`src/nodeEditor/NodeEditorModel.hpp/cpp`**
   - Added `computeLayout()` for instant layout
   - Added `computeLayoutAnimated()` for frame-by-frame animation
   - Added `isLayoutAnimating()` to check animation state
   - Added `resetLayout()` to trigger new animations
   - Fixed initialization logic to avoid double-init

3. **`src/Gui.hpp/cpp`**
   - Added `animateLayout` flag (default: true)
   - Added `layoutComputed` flag to track state
   - Added toolbar with buttons and checkbox
   - Added animation loop that runs per-frame
   - Added visual "[Animating...]" indicator

## How It Works

### When Node Editor Opens

```
User opens app
    ↓
Gui::drawNodeEditor() called every frame
    ↓
First frame: nodeEditorDirty = true
    ↓
buildFromSynth(synthPtr)
    - Creates nodes list from synthPtr->modPtrs
    - layoutInitialized = false
    ↓
animateLayout = true (default)
layoutComputed = false
    ↓
Next frame and onwards...
```

### Animation Loop (Every Frame)

```cpp
if (animateLayout && !layoutComputed) {
    nodeEditorModel.computeLayoutAnimated();
    //   ↓
    // First call: layoutEngine.initialize(synthPtr)
    //   - Creates LayoutNodes with random positions
    //   - Discovers connections from Mod::connections
    //   
    // Each call: layoutEngine.step()
    //   - Applies forces (repulsion, springs, center)
    //   - Updates positions
    //   - Returns false when stable
    //   
    // Updates ImNodes positions:
    //   ImNodes::SetNodeGridSpacePos(nodeId, pos)
    
    if (!nodeEditorModel.isLayoutAnimating()) {
        layoutComputed = true;  // Stop animating
    }
}
```

### Force Simulation

Each frame the layout engine:

1. **Repulsion**: All nodes push each other away
   ```
   force = repulsionStrength / distance²
   ```

2. **Springs**: Connected nodes pull together
   ```
   force = springStrength * (distance - springLength)
   ```

3. **Center**: Gentle pull toward center
   ```
   force = centerAttraction * (center - position)
   ```

4. **Damping**: Slow down movement
   ```
   velocity *= damping
   ```

5. **Update**: Move nodes
   ```
   position += velocity
   ```

## User Controls

### Toolbar Buttons

```
┌─────────────────────────────────────────────────────┐
│ [Compute Layout] [Animate Layout] ☑ Auto-animate  │
│                                    [Animating...]   │
├─────────────────────────────────────────────────────┤
│                                                      │
│   Node graph appears here...                        │
│                                                      │
└─────────────────────────────────────────────────────┘
```

**"Compute Layout"**
- Computes all iterations instantly
- No animation
- Fast for large graphs

**"Animate Layout"**
- Resets layout state
- Triggers new animation from random positions
- Fun to watch

**"Auto-animate" checkbox**
- ON (default): Animates on first load
- OFF: Manual layout only

## Code Flow Diagram

```
┌──────────────┐
│   Synth      │  Contains modPtrs map
└──────┬───────┘
       │
       ↓ buildFromSynth()
┌──────────────┐
│NodeEditorModel│  Creates nodes list
└──────┬───────┘
       │
       ↓ computeLayoutAnimated()
┌──────────────┐
│NodeEditorLayout│  Force-directed simulation
└──────┬───────┘
       │
       ↓ getNodePosition()
┌──────────────┐
│   ImNodes    │  Visual rendering
└──────────────┘
```

## Integration Example

### Minimal Setup (Already Done in Gui.cpp)

```cpp
class Gui {
private:
    NodeEditorModel nodeEditorModel;
    bool nodeEditorDirty { true };
    bool animateLayout { true };
    bool layoutComputed { false };
};

void Gui::drawNodeEditor() {
    // 1. Rebuild if needed
    if (nodeEditorDirty) {
        nodeEditorModel.buildFromSynth(synthPtr);
        nodeEditorDirty = false;
        layoutComputed = false;
    }

    ImGui::Begin("NodeEditor");
    
    // 2. Toolbar
    if (ImGui::Button("Compute Layout")) {
        nodeEditorModel.computeLayout();
        layoutComputed = true;
        animateLayout = false;
    }
    // ... more buttons ...
    
    // 3. Animation loop
    if (animateLayout && !layoutComputed) {
        nodeEditorModel.computeLayoutAnimated();
        if (!nodeEditorModel.isLayoutAnimating()) {
            layoutComputed = true;
        }
    }
    
    // 4. Draw nodes
    ImNodes::BeginNodeEditor();
    for (const auto& node : nodeEditorModel.nodes) {
        // Draw node...
    }
    ImNodes::EndNodeEditor();
    
    ImGui::End();
}
```

## Customizing Animation

### Change Speed

In `NodeEditorLayout::Config`:

```cpp
// Faster animation
config.damping = 0.7f;        // Less damping
config.maxSpeed = 100.0f;     // Faster movement
config.stopThreshold = 1.0f;  // Looser convergence

// Slower, smoother animation
config.damping = 0.95f;       // More damping
config.maxSpeed = 30.0f;      // Slower movement
config.stopThreshold = 0.1f;  // Tighter convergence
```

### Change Spacing

```cpp
// More spread out
config.repulsionStrength = 2000.0f;  // Stronger repulsion
config.springLength = 300.0f;        // Longer connections

// More compact
config.repulsionStrength = 500.0f;   // Weaker repulsion
config.springLength = 150.0f;        // Shorter connections
```

## Testing

### Build and Run

```bash
cd example_simple
make clean && make
./bin/example_simple
```

### What You Should See

1. App launches
2. Node Editor window opens
3. Nodes appear in random positions
4. Nodes animate into organized layout over ~2-3 seconds
5. "[Animating...]" indicator shows, then disappears
6. Final layout with nodes nicely spaced

### Interaction

- **Option-drag**: Pan the canvas
- **Mouse wheel**: Zoom
- **Drag nodes**: Move manually
- **Mini-map**: Bottom-right corner

## Troubleshooting

**Q: Nodes never stop moving**  
A: Check `stopThreshold` - try increasing to 1.0

**Q: Animation too fast/slow**  
A: Adjust `damping` (0.7-0.95) and `maxSpeed` (30-100)

**Q: Nodes overlap**  
A: Increase `repulsionStrength` to 2000-3000

**Q: Nodes spread too far**  
A: Decrease `repulsionStrength` or increase `centerAttraction`

**Q: "[Animating...]" stays forever**  
A: Layout may not be converging. Check `maxIterations` is being reached.

## Next Steps

1. ✅ **Basic animation working** - You are here!
2. **Save/load positions** - Persist node positions to JSON
3. **Manual positioning** - Remember user-dragged positions
4. **Edit mode** - Add/remove connections interactively
5. **Advanced layouts** - Hierarchical, circular, force-atlas, etc.

## Files to Review

- `NodeEditor-Animation-Integration.md` - Full documentation
- `src/nodeEditor/NodeEditorLayout.cpp` - Layout algorithm
- `src/nodeEditor/NodeEditorModel.cpp` - Model logic
- `src/Gui.cpp` (drawNodeEditor) - Integration code
