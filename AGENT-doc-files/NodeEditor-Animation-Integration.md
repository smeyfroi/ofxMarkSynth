# Node Editor Animated Layout Integration

## Overview

The node editor now supports animated force-directed layout of the Synth's Mod graph. The layout engine works directly with the Synth's `modPtrs` and automatically discovers connections.

## Key Components

### 1. NodeEditorLayout
**Location:** `src/nodeEditor/NodeEditorLayout.hpp/cpp`

Implements force-directed graph layout algorithm:
- **Repulsion forces**: Nodes push each other away (prevents overlap)
- **Spring forces**: Connected nodes pull together (along connections)
- **Center attraction**: Gentle pull toward center (prevents drift)
- **Boundary forces**: Keeps nodes within canvas bounds

**Simplified API:**
```cpp
// Initialize directly from Synth
layoutEngine.initialize(synthPtr, bounds);

// Compute layout instantly
layoutEngine.compute();

// Or animate with single steps
while (layoutEngine.step()) {
    // Apply positions each frame
}

// Check convergence
bool done = layoutEngine.isStable();
```

### 2. NodeEditorModel
**Location:** `src/nodeEditor/NodeEditorModel.hpp/cpp`

Manages the node graph data and layout state:

```cpp
class NodeEditorModel {
public:
    void buildFromSynth(const std::shared_ptr<Synth> synthPtr);
    
    // Layout methods
    void computeLayout();               // Instant layout
    void computeLayoutAnimated();       // Single animation step
    bool isLayoutAnimating() const;     // Check if still animating
    void resetLayout();                 // Reset for new animation
    
    std::shared_ptr<Synth> synthPtr;
    std::vector<NodeEditorNode> nodes;
};
```

### 3. Gui Integration
**Location:** `src/Gui.hpp/cpp`

The GUI now includes:
- **Auto-animate checkbox**: Toggle automatic layout animation
- **"Compute Layout" button**: Instant layout computation
- **"Animate Layout" button**: Trigger new animation
- **Status indicator**: Shows "[Animating...]" while running

## Usage in drawNodeEditor()

```cpp
void Gui::drawNodeEditor() {
    // Rebuild if Synth changes
    if (nodeEditorDirty) {
        nodeEditorModel.buildFromSynth(synthPtr);
        nodeEditorDirty = false;
        layoutComputed = false;
    }

    ImGui::Begin("NodeEditor");
    
    // === TOOLBAR ===
    if (ImGui::Button("Compute Layout")) {
        nodeEditorModel.computeLayout();      // Instant
        layoutComputed = true;
        animateLayout = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Animate Layout")) {
        nodeEditorModel.resetLayout();        // Reset state
        layoutComputed = false;
        animateLayout = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-animate", &animateLayout);
    
    // Status indicator
    if (nodeEditorModel.isLayoutAnimating()) {
        ImGui::SameLine();
        ImGui::TextColored(GREEN_COLOR, "[Animating...]");
    }
    
    ImGui::Separator();
    
    // === ANIMATION LOOP ===
    // Run one step per frame if enabled
    if (animateLayout && !layoutComputed) {
        nodeEditorModel.computeLayoutAnimated();
        if (!nodeEditorModel.isLayoutAnimating()) {
            layoutComputed = true;  // Animation finished
        }
    }
    
    // === DRAW NODES ===
    ImNodes::BeginNodeEditor();
    
    // ... draw nodes and links ...
    
    ImNodes::EndNodeEditor();
    ImGui::End();
}
```

## How Animation Works

### Frame-by-Frame Process

1. **Initialization** (first frame when dirty):
   ```cpp
   nodeEditorModel.buildFromSynth(synthPtr);
   // nodes created but not positioned yet
   ```

2. **Layout starts** (when animateLayout = true):
   ```cpp
   nodeEditorModel.computeLayoutAnimated();
   // -> layoutEngine.initialize(synthPtr, bounds)
   //    - Creates LayoutNodes for each Mod
   //    - Random initial positions
   //    - Discovers connections from Mod.connections
   ```

3. **Each frame** (while animating):
   ```cpp
   nodeEditorModel.computeLayoutAnimated();
   // -> layoutEngine.step()
   //    - Applies forces (repulsion, springs, center)
   //    - Updates positions with damping
   //    - Returns true while still moving
   //    
   // -> Updates imnodes positions:
   //    ImNodes::SetNodeGridSpacePos(nodeId, position)
   ```

4. **Convergence** (when stable):
   ```cpp
   if (!nodeEditorModel.isLayoutAnimating()) {
       layoutComputed = true;  // Stop animating
   }
   ```

### Animation Parameters

Adjust in `NodeEditorLayout::Config`:

```cpp
struct Config {
    float repulsionStrength { 1000.0f };  // Higher = more spacing
    float springStrength { 0.01f };       // Higher = tighter connections
    float springLength { 200.0f };        // Ideal connection distance
    float damping { 0.85f };              // Higher = slower convergence
    float maxSpeed { 50.0f };             // Cap movement per frame
    float stopThreshold { 0.5f };         // When to stop animating
    int maxIterations { 300 };            // Safety limit
    glm::vec2 centerAttraction { 0.001f }; // Pull toward center
};
```

**Tuning Tips:**
- **More spacing**: Increase `repulsionStrength` to 2000-3000
- **Tighter graphs**: Increase `springStrength` to 0.02-0.05
- **Faster animation**: Decrease `damping` to 0.7-0.8
- **Slower, smoother**: Increase `damping` to 0.9-0.95
- **Larger graphs**: Increase `springLength` to 300-400

## User Experience

### Default Behavior
- **On load**: Graph animates from random positions to stable layout
- **Auto-animate**: Enabled by default
- **Visual feedback**: Green "[Animating...]" indicator

### User Controls
1. **Auto-animate checkbox**: 
   - When OFF: Graph doesn't auto-layout
   - When ON: New graphs animate on load

2. **"Compute Layout" button**:
   - Instantly computes final positions
   - No animation
   - Good for large graphs

3. **"Animate Layout" button**:
   - Resets and triggers new animation
   - Fun to watch
   - Good for reorganizing after manual edits (future)

### Manual Override
User can drag nodes during/after animation:
- imnodes handles manual positioning
- Manual positions override computed positions
- Re-trigger animation to reset

## Integration Checklist

- [x] `NodeEditorLayout` works directly with Synth
- [x] `NodeEditorModel` has animation methods
- [x] `Gui` has toolbar controls
- [x] Animation state tracked (`layoutComputed`, `animateLayout`)
- [x] Visual feedback during animation
- [x] User controls for instant vs animated layout
- [ ] Persist node positions (future: save/load)
- [ ] Edit mode (future: add/remove connections)

## Example: Custom Animation Speed

To make animation faster/slower, modify in your app setup:

```cpp
// In Gui::setup() or similar:
NodeEditorLayout::Config customConfig;
customConfig.damping = 0.7f;        // Faster (less damping)
customConfig.maxSpeed = 100.0f;     // Allow faster movement
customConfig.stopThreshold = 1.0f;  // Looser convergence

// Pass to model (requires adding config parameter to NodeEditorModel)
```

## Debugging

Enable debug output in `NodeEditorLayout::step()`:

```cpp
bool NodeEditorLayout::step() {
    if (currentIteration >= config.maxIterations) return false;
    
    applyForces();
    updatePositions();
    
    currentIteration++;
    
    // Debug output
    if (currentIteration % 10 == 0) {
        ofLogNotice() << "Layout iteration " << currentIteration 
                      << ", stable: " << isStable();
    }
    
    return !isStable();
}
```

## Performance Notes

- **Small graphs (< 10 nodes)**: Animation takes ~50-100 frames (~1-2 seconds)
- **Medium graphs (10-30 nodes)**: ~100-200 frames (~2-4 seconds)
- **Large graphs (> 30 nodes)**: Consider instant layout for better UX

The algorithm is O(N²) for repulsion, O(E) for springs where:
- N = number of nodes
- E = number of connections

For very large graphs (> 50 nodes), consider hierarchical layout instead.

## Future Enhancements

1. **Save/Load Positions**: Persist node positions to JSON
2. **Manual Override**: Remember user-dragged positions
3. **Hierarchical Layout**: Alternative layout for large graphs
4. **Layer-aware Layout**: Group by layer/type
5. **Connection Routing**: Curved/orthogonal edge routing
6. **Collision Detection**: Better node overlap prevention
7. **Zoom-to-fit**: Auto-frame all nodes
8. **Mini-map sync**: Update mini-map during animation

## Troubleshooting

**Animation never stops:**
- Check `stopThreshold` - may be too low
- Verify `maxIterations` is being reached
- Look for numerical instability in forces

**Nodes overlap:**
- Increase `repulsionStrength`
- Decrease `springLength`
- Check for duplicate nodes

**Nodes fly off screen:**
- Increase `damping` (slow down)
- Decrease `maxSpeed`
- Check `centerAttraction` is non-zero

**Animation too slow:**
- Decrease `damping`
- Increase `maxSpeed`
- Loosen `stopThreshold`
