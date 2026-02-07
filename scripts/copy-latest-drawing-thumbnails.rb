#!/usr/bin/env ruby
# frozen_string_literal: true

# copy-latest-drawing-thumbnails.rb
#
# For each immediate subfolder under <artefact_drawing_dir>, find the newest
# JPEG anywhere within that subfolder tree, then write a 256x256 (crop-to-fill)
# thumbnail into <config_synth_dir> named <subfolder>.jpg.
#
# Example output:
#   <config_synth_dir>/00-minimal-av-softmarks.jpg
#
# Requires: macOS `sips`.
#
# Usage:
#   ./copy-latest-drawing-thumbnails.rb <artefact_drawing_dir> <config_synth_dir>

require "fileutils"
require "open3"
require "pathname"

SIZE = 256

def usage!(message = nil)
  warn "Error: #{message}\n" if message
  warn <<~EOF
    Usage:
      #{File.basename($PROGRAM_NAME)} <artefact_drawing_dir> <config_synth_dir>

    Example:
      #{File.basename($PROGRAM_NAME)} \
        "/Users/steve/Documents/MarkSynth-performances/Improvisation1/artefact/drawing" \
        "/Users/steve/Documents/MarkSynth-performances/Improvisation1/config/synth"
  EOF
  exit 1
end

def have_cmd?(name)
  system(name, "--version", out: File::NULL, err: File::NULL)
end

def parse_sips_dimensions(path)
  stdout, status = Open3.capture2(
    "sips",
    "-g",
    "pixelWidth",
    "-g",
    "pixelHeight",
    "-1",
    path.to_s
  )
  raise "Failed to read dimensions via sips: #{path}" unless status.success?

  width = stdout[/pixelWidth:\s*(\d+)/, 1]&.to_i
  height = stdout[/pixelHeight:\s*(\d+)/, 1]&.to_i
  unless width && height && width.positive? && height.positive?
    raise "Could not parse pixelWidth/pixelHeight for: #{path}"
  end

  [width, height]
end

def newest_jpeg_under(dir)
  candidates = Dir.glob(dir.join("**", "*").to_s).select do |p|
    File.file?(p) && p.match?(/\.(jpe?g)\z/i)
  end
  return nil if candidates.empty?

  candidates.max_by { |p| File.mtime(p) }
end

def sips_crop_to_fill(src_path, dst_path, size)
  width, height = parse_sips_dimensions(src_path)

  # Scale so the smaller dimension becomes `size`.
  scale = size.to_f / [width, height].min
  new_w = (width * scale).round
  new_h = (height * scale).round

  # Center crop.
  offset_x = [(new_w - size) / 2, 0].max
  offset_y = [(new_h - size) / 2, 0].max

  ok = system(
    "sips",
    "--resampleHeightWidth",
    new_h.to_s,
    new_w.to_s,
    "--cropToHeightWidth",
    size.to_s,
    size.to_s,
    "--cropOffset",
    offset_y.to_s, # from top
    offset_x.to_s, # from left
    "-s",
    "format",
    "jpeg",
    src_path.to_s,
    "--out",
    dst_path.to_s,
    out: File::NULL
  )

  raise "sips resize/crop failed for: #{src_path}" unless ok
end

usage!("missing arguments") unless ARGV.length == 2

src_root = Pathname.new(ARGV[0])
dst_dir = Pathname.new(ARGV[1])

usage!("source is not a directory: #{src_root}") unless src_root.directory?

unless have_cmd?("sips")
  warn "Error: this script requires `sips` (built into macOS)."
  exit 3
end

FileUtils.mkdir_p(dst_dir)

subdirs = src_root.children.select(&:directory?).sort_by { |p| p.basename.to_s }

processed = 0
subdirs.each do |dir|
  newest = newest_jpeg_under(dir)
  if newest.nil?
    warn "Skip (no jpeg): #{dir}"
    next
  end

  out_path = dst_dir.join("#{dir.basename}.jpg")
  sips_crop_to_fill(Pathname.new(newest), out_path, SIZE)
  puts "Wrote: #{out_path}  (from #{newest})"
  processed += 1
end

warn "Done. Processed #{processed} folder(s)."
