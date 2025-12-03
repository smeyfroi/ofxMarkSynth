#!/bin/bash
#
# mux-audio-video.sh
#
# Muxes audio and video files that share the same timestamp into a single file.
# Expects files named:
#   drawing-{timestamp}.mp4  (video)
#   audio-{timestamp}.wav    (audio)
#
# Output:
#   drawing-{timestamp}-muxed.mp4
#
# Usage:
#   ./mux-audio-video.sh <directory>
#   ./mux-audio-video.sh <video_file> <audio_file>
#
# Examples:
#   ./mux-audio-video.sh ~/artefacts/drawing-recording/MySynth/
#   ./mux-audio-video.sh drawing-2025-01-15-12-30-00.mp4 audio-2025-01-15-12-30-00.wav
#

set -e

# Check for ffmpeg
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: ffmpeg is required but not installed."
    echo "Install with: brew install ffmpeg"
    exit 1
fi

mux_files() {
    local video_file="$1"
    local audio_file="$2"
    local output_file="${video_file%.mp4}-muxed.mp4"
    
    if [[ ! -f "$video_file" ]]; then
        echo "Error: Video file not found: $video_file"
        return 1
    fi
    
    if [[ ! -f "$audio_file" ]]; then
        echo "Error: Audio file not found: $audio_file"
        return 1
    fi
    
    echo "Muxing:"
    echo "  Video: $video_file"
    echo "  Audio: $audio_file"
    echo "  Output: $output_file"
    
    ffmpeg -y -i "$video_file" -i "$audio_file" \
        -c:v copy \
        -c:a aac -b:a 192k \
        -shortest \
        "$output_file"
    
    echo "Created: $output_file"
    echo ""
}

process_directory() {
    local dir="$1"
    local count=0
    
    echo "Scanning directory: $dir"
    echo ""
    
    # Find all video files and match with audio files
    for video_file in "$dir"/drawing-*.mp4; do
        [[ -f "$video_file" ]] || continue
        
        # Skip already muxed files
        if [[ "$video_file" == *-muxed.mp4 ]]; then
            continue
        fi
        
        # Extract timestamp from video filename
        local basename=$(basename "$video_file")
        local timestamp="${basename#drawing-}"
        timestamp="${timestamp%.mp4}"
        
        # Find matching audio file
        local audio_file="$dir/audio-${timestamp}.wav"
        
        if [[ -f "$audio_file" ]]; then
            mux_files "$video_file" "$audio_file"
            ((count++))
        else
            echo "Warning: No matching audio file for $video_file"
            echo "  Expected: $audio_file"
            echo ""
        fi
    done
    
    if [[ $count -eq 0 ]]; then
        echo "No matching video/audio pairs found."
    else
        echo "Muxed $count file(s)."
    fi
}

# Main
if [[ $# -eq 0 ]]; then
    echo "Usage:"
    echo "  $0 <directory>              - Process all matching pairs in directory"
    echo "  $0 <video.mp4> <audio.wav>  - Mux specific files"
    exit 1
fi

if [[ $# -eq 1 ]]; then
    if [[ -d "$1" ]]; then
        process_directory "$1"
    else
        echo "Error: '$1' is not a directory"
        echo "For single file muxing, provide both video and audio files."
        exit 1
    fi
elif [[ $# -eq 2 ]]; then
    mux_files "$1" "$2"
else
    echo "Error: Too many arguments"
    exit 1
fi
