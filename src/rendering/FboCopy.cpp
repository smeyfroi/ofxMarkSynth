#include "rendering/FboCopy.hpp"


namespace ofxMarkSynth {


// Allocates 'dst' to match 'src' (size, color attachments, target, filters).
static void ensureAllocatedLike(const ofFbo& src, ofFbo& dst, bool wantDepth = false) {
  if (!src.isAllocated()) return;

  const int w = src.getWidth();
  const int h = src.getHeight();
  const int numColors = std::max(1, src.getNumTextures());
  const auto& srcTex0 = src.getTexture(0).getTextureData();

  bool needsAlloc = !dst.isAllocated()
                 || dst.getWidth()  != w
                 || dst.getHeight() != h
                 || dst.getNumTextures() != numColors
                 || dst.getTexture(0).getTextureData().textureTarget    != srcTex0.textureTarget
                 || dst.getTexture(0).getTextureData().glInternalFormat != srcTex0.glInternalFormat;

  if (needsAlloc) {
    ofFbo::Settings s;
    s.width  = w;
    s.height = h;
    s.numColorbuffers = numColors;
    s.internalformat  = srcTex0.glInternalFormat;   // assumes same format across attachments
    s.textureTarget   = srcTex0.textureTarget;
    s.useDepth        = wantDepth && src.getDepthTexture().isAllocated();
    s.useStencil      = false;
    s.depthStencilAsTexture = s.useDepth && src.getDepthTexture().getTextureData().bAllocated;
    s.minFilter = GL_LINEAR; // GL_NEAREST; //****** TODO CHECK THIS
    s.maxFilter = GL_LINEAR; // GL_NEAREST;
    s.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    s.wrapModeVertical   = GL_CLAMP_TO_EDGE;
    dst.allocate(s);
  }
}

// Pure oF path: render-copy each color attachment (GPU only).
void fboCopyDraw(const ofFbo& src, ofFbo& dst) {
  ensureAllocatedLike(src, dst, /*wantDepth=*/false);

  const int w = src.getWidth();
  const int h = src.getHeight();
  const int num = std::min(src.getNumTextures(), dst.getNumTextures());

  dst.begin();
  ofDisableBlendMode();
  ofSetColor(255);
  for (int i = 0; i < num; ++i) {
    dst.setActiveDrawBuffer(i);
    // No clear to keep previous content if dst has more attachments.
    src.getTexture(i).draw(0, 0, w, h);
  }
  dst.end();
}

// GL blit path: fast, preserves formats; can also copy depth.
void fboCopyBlit(const ofFbo& src, ofFbo& dst, bool copyDepth) {
  ensureAllocatedLike(src, dst, /*wantDepth=*/copyDepth);

  // Save state
  GLint prevReadFbo = 0, prevDrawFbo = 0, prevReadBuf = 0, prevDrawBuf = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);
  glGetIntegerv(GL_READ_BUFFER, &prevReadBuf);
  glGetIntegerv(GL_DRAW_BUFFER, &prevDrawBuf);
  GLboolean scissorEnabled = GL_FALSE;
  glGetBooleanv(GL_SCISSOR_TEST, &scissorEnabled);
  if (scissorEnabled) glDisable(GL_SCISSOR_TEST);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, src.getId());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.getId());

  const int num = std::min(src.getNumTextures(), dst.getNumTextures());
  for (int i = 0; i < num; ++i) {
    glReadBuffer(GL_COLOR_ATTACHMENT0 + i);      // select source buffer i
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);      // select exactly one dest buffer
    glBlitFramebuffer(0, 0, src.getWidth(), src.getHeight(),
                      0, 0, dst.getWidth(), dst.getHeight(),
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }

  if (copyDepth) {
    glReadBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    glBlitFramebuffer(0, 0, src.getWidth(), src.getHeight(),
                      0, 0, dst.getWidth(), dst.getHeight(),
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  // Restore state.
  if (scissorEnabled) glEnable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
  glReadBuffer(prevReadBuf);
  glDrawBuffer(prevDrawBuf);
}


} // namespace ofxMarkSynth
