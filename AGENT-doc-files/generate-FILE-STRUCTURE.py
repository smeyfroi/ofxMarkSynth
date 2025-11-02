#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
File Structure Generator Script
-------------------------------

This script generates a markdown file named `FILE-STRUCTURE.md` that contains
a recursive tree representation of a target directory.

Author:
moebiusSurfing (https://github.com/moebiussurfing)
Date: 2025-10-10

#--

Usage:
# place this script in your project root (where .gitignore lives)
generate-FILE-STRUCTURE.py

# Analyze current directory (default)
python generate-FILE-STRUCTURE.py

# Analyze a subdirectory (e.g., 'docs')
python generate-FILE-STRUCTURE.py docs

# Analyze an absolute path
python generate-FILE-STRUCTURE.py /path/to/project/docs

#--

Key Features:
- Can be run from the project root to analyze any subdirectory (e.g., /docs).
- Always reads `.gitignore` from the **project root** (where this script resides),
  not from the target directory. This ensures consistent exclusion rules across the project.
- **NEW**: Supports forced inclusions that override ALL exclusion rules.
- Excludes directories and files based on:
a) A hardcoded list of common build/cache/IDE folders and files.
b) File extensions (e.g., .dll, .exe, .log).
c) Patterns from the root-level `.gitignore` file (if it exists).
- Prints a beautifully colored tree directly to the console using ANSI escape codes.
- Saves the same structure in `FILE-STRUCTURE.md` inside the **target directory**.
- Supports command-line argument to specify the target directory (defaults to current directory).

Hardcoded exclusions include:
- Common VCS folders (.git)
- Build/output directories (bin, obj, dist, build, dll)
- Dependency folders (node_modules, venv, .venv)
- IDE/editor metadata (.idea, .vscode)
- OS-specific junk (.DS_Store, Thumbs.db)
- Specific files (addons.make, .gitignore itself)
- Binary/compiled file extensions (.dll, .exe, .so, etc.)

Forced inclusions override all exclusions:
- Files listed in FORCED_INCLUSIONS will ALWAYS appear in the output,
even if they match .gitignore patterns, hardcoded exclusions, or extension filters.

Note on .gitignore handling:
- The script assumes `.gitignore` is located in the same directory as this script (project root).
- It does NOT look for `.gitignore` files in subdirectories.
- Parsing is simplified: supports basic globs (*, ?), directory markers (/), and path patterns.
It does NOT fully implement Git's spec (e.g., no support for **, complex negations with !),
but covers 95% of real-world use cases.
"""

import os
import sys
import fnmatch
import logging
from pathlib import Path
from typing import List, Set
import argparse

# ----------------------------
# Configuration & Constants
# ----------------------------

# NEW: Forced inclusions - these files will ALWAYS be included,
# regardless of .gitignore, hardcoded exclusions, or extension filters.
# Match is case-insensitive and based on basename only.
FORCED_INCLUSIONS = [
	'addons.make',
	'Capture.PNG',
	'Capture-v0.1.PNG',
]

# Hardcoded list of file/directory names to exclude (case-insensitive).
# These are matched against the basename of each file/directory.
HARDCODED_EXCLUSIONS_NAMES = [
	'.git',
	'bin',
	'obj',
	'__pycache__',
	'.venv',
	'venv',
	'node_modules',
	'dist',
	'build',
	'.idea',
	'.vscode',
	'.DS_Store',
	'Thumbs.db',
	'.gitignore',           # Exclude the .gitignore file itself from output
	'dll',                  # Exclude the 'dll' directory entirely (not just its contents)
	'refresh_gitignore.ps1'
]

# Hardcoded glob-like patterns for path-based exclusions.
# These are matched against the full relative path (e.g., 'dll/some.dll').
# Only simple patterns like 'prefix/*' are supported.
HARDCODED_EXCLUSIONS_GLOBS = [
	'dll/*',  # Exclude all contents inside any 'dll' directory
]

# Hardcoded file extensions to exclude (without the dot, lowercase).
# Applied only to files (not directories).
HARDCODED_EXCLUDED_EXTENSIONS = {
	'dll',
	'exe',
	'log',
	'tmp',
	'bak',
	'pyc',
	'o',
	'obj',      # compiled object files (not to be confused with /obj directory)
	'so',
	'dylib',
	'pdb',
	'ilk',
	'idb',
	'sdf',
	'opensdf',
	'suo',
	'user',
	'cache',
	'min',
	'map',
	'swp',
	'swo',
	'class',
	'jar',
	'war',
	'ear',
	'zip',
	'tar',
	'gz',
	'rar',
	'7z',
}

# Output filename that will be created in the target directory
OUTPUT_FILENAME = "FILE-STRUCTURE.md"

# ANSI color codes for pretty console output
COLORS = {
	'dir': '\033[34m',      # Blue for directories
	'file': '\033[37m',     # Light gray for files
	'root': '\033[36m',     # Cyan for root name
	'connector': '\033[33m', # Yellow for tree connectors (├──, └──)
	'forced': '\033[32m',   # Green for forced inclusions
	'reset': '\033[0m',     # Reset to default
}

# ----------------------------
# Logging Setup (Colored Console Output)
# ----------------------------

class ColoredFormatter(logging.Formatter):
	"""
Custom logging formatter that adds color to log level names and messages.
Uses ANSI escape codes for terminal colorization.
	"""
	LOG_COLORS = {
		'DEBUG': '\033[36m',    # Cyan
		'INFO': '\033[32m',     # Green
		'WARNING': '\033[33m',  # Yellow
		'ERROR': '\033[31m',    # Red
		'CRITICAL': '\033[35m', # Magenta
	}
	RESET = '\033[0m'

	def format(self, record):
		"""Apply color to log level and message."""
		log_color = self.LOG_COLORS.get(record.levelname, self.RESET)
		record.levelname = f"{log_color}{record.levelname}{self.RESET}"
		record.msg = f"{log_color}{record.msg}{self.RESET}"
		return super().format(record)

# Configure root logger
logger = logging.getLogger("FileStructureGenerator")
logger.setLevel(logging.DEBUG)

# Console handler with colored output
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.DEBUG)
console_handler.setFormatter(ColoredFormatter('%(levelname)s: %(message)s'))
logger.addHandler(console_handler)

# ----------------------------
# Helper Functions
# ----------------------------

def normalize_name(name: str) -> str:
	"""
Normalize a file/directory name for case-insensitive comparison.
On Windows and macOS, filesystems are case-insensitive, so we lower-case names.
This ensures consistent exclusion matching across platforms.

Args:
name (str): Original name from filesystem.

Returns:
str: Lowercase version of the name.
	"""
	return name.lower()

def is_forced_inclusion(basename: str, forced_inclusions: Set[str]) -> bool:
	"""
Check if a file/directory is in the forced inclusions list.

Args:
basename (str): Name of the file/directory.
forced_inclusions (Set[str]): Set of normalized forced inclusion names.

Returns:
bool: True if this item should be forcibly included.
	"""
	return normalize_name(basename) in forced_inclusions

def parse_gitignore(gitignore_path: Path) -> List[str]:
	"""
Parse a .gitignore file and return a cleaned list of patterns.

Processing steps:
- Skip empty lines and comments (lines starting with '#').
- Strip trailing whitespace.
- Remove leading '/' to treat all patterns as relative (since we walk recursively).
- Preserve trailing '/' to indicate directory-only patterns.

Note: This is a simplified parser. It does not handle:
- Negations (!pattern)
- Double asterisks (**)
- Anchored patterns beyond leading slash removal
- Complex escape sequences

Args:
gitignore_path (Path): Path to the .gitignore file.

Returns:
List[str]: List of cleaned patterns ready for fnmatch.
	"""
	patterns = []
	try:
		with open(gitignore_path, 'r', encoding='utf-8', errors='ignore') as f:
			for line_num, line in enumerate(f, 1):
				stripped_line = line.strip()
				# Skip comments and blank lines
				if not stripped_line or stripped_line.startswith('#'):
					continue
				# Remove trailing whitespace (including \r for Windows)
				cleaned = stripped_line.rstrip()
				# Remove leading slash: Git treats /foo as root-anchored,
				# but for recursive walking, we treat all as relative.
				if cleaned.startswith('/'):
					cleaned = cleaned[1:]
				patterns.append(cleaned)
		logger.info(f"Successfully parsed .gitignore at {gitignore_path} with {len(patterns)} patterns.")
	except Exception as e:
		logger.error(f"Failed to read .gitignore at {gitignore_path}: {e}")
		return []
	return patterns

def path_matches_hardcoded_glob(relative_path: str, glob_pattern: str) -> bool:
	"""
Check if a relative path matches a simple hardcoded glob pattern.

Currently supports only patterns of the form 'dirname/*', which means:
"exclude everything inside a directory named 'dirname'".

Example:
pattern = 'dll/*'
path = 'dll/helper.dll' → matches
path = 'other/dll/file.txt' → does NOT match (only top-level 'dll')

Note: This is intentionally simple. For full glob support, use glob or pathspec.

Args:
relative_path (str): Path relative to project root (with forward slashes).
glob_pattern (str): Glob pattern like 'dll/*'.

Returns:
bool: True if the path matches the glob pattern.
	"""
	if glob_pattern.endswith('/*'):
		dirname = glob_pattern[:-2]  # Remove '/*' → 'dll'
		# Match only if path starts with 'dll/' (i.e., immediate child of root)
		if relative_path == dirname:
			# Edge case: someone named a file 'dll' (not dir) – but we exclude it anyway
			return True
		if relative_path.startswith(dirname + '/'):
			return True
	return False

def should_exclude(
		relative_path: str,
		basename: str,
		is_directory: bool,
		hardcoded_names: Set[str],
		hardcoded_globs: List[str],
		gitignore_patterns: List[str],
		excluded_extensions: Set[str],
		forced_inclusions: Set[str]
) -> bool:
	"""
Determine whether a file or directory should be excluded from the tree.

Checks in order:
0. FORCED INCLUSIONS (if matched, return False immediately - never exclude)
1. File extension (if it's a file)
2. Hardcoded name exclusions (e.g., 'node_modules', '.git')
3. Hardcoded glob exclusions (e.g., 'dll/*')
4. .gitignore patterns (from project root)

Args:
relative_path (str): Path relative to project root (e.g., 'docs/index.md')
basename (str): Name of the file/directory (e.g., 'index.md')
is_directory (bool): True if this is a directory
hardcoded_names (Set[str]): Set of normalized names to exclude
hardcoded_globs (List[str]): List of glob patterns to exclude
gitignore_patterns (List[str]): Patterns from .gitignore
excluded_extensions (Set[str]): Set of file extensions to exclude (without dot)
forced_inclusions (Set[str]): Set of normalized names to ALWAYS include

Returns:
bool: True if the item should be excluded, False otherwise.
	"""
	# Check 0: FORCED INCLUSIONS (highest priority - overrides everything)
	if is_forced_inclusion(basename, forced_inclusions):
		logger.debug(f"FORCED INCLUSION (overrides all exclusions): {relative_path}")
		return False

	# Check 1: File extension (only for files)
	if not is_directory:
		# Extract extension: split by '.' and take last part; handle hidden files like .gitignore
		parts = basename.split('.')
		if len(parts) > 1:
			# For files like "archive.tar.gz", we only check the last extension ("gz")
			ext = normalize_name(parts[-1])
			if ext in excluded_extensions:
				logger.debug(f"Excluded by extension '.{ext}': {relative_path}")
				return True

	# Check 2: Hardcoded name exclusions (case-insensitive)
	if normalize_name(basename) in hardcoded_names:
		logger.debug(f"Excluded by hardcoded name: {relative_path}")
		return True

	# Check 3: Hardcoded glob exclusions
	for glob in hardcoded_globs:
		if path_matches_hardcoded_glob(relative_path, glob):
			logger.debug(f"Excluded by hardcoded glob '{glob}': {relative_path}")
			return True

	# Check 4: .gitignore patterns
	for pattern in gitignore_patterns:
		# Handle directory-only patterns (ending with /)
		if pattern.endswith('/'):
			dir_pattern = pattern.rstrip('/')
			if is_directory and (
				fnmatch.fnmatch(basename, dir_pattern) or
				fnmatch.fnmatch(relative_path, dir_pattern) or
				(relative_path + '/').startswith(pattern)
			):
				logger.debug(f"Excluded by .gitignore (directory pattern): {relative_path}")
				return True
		else:
			# Match against basename or full relative path
			if (
				fnmatch.fnmatch(basename, pattern) or
				fnmatch.fnmatch(relative_path, pattern)
			):
				logger.debug(f"Excluded by .gitignore (file/dir pattern): {relative_path}")
				return True
			# Handle path patterns (contain '/')
			if '/' in pattern and fnmatch.fnmatch(relative_path, pattern):
				logger.debug(f"Excluded by .gitignore (path pattern): {relative_path}")
				return True

	return False

def build_tree_entries(
		current_path: Path,
		project_root: Path,
		prefix: str = "",
		hardcoded_names: Set[str] = None,
		hardcoded_globs: List[str] = None,
		gitignore_patterns: List[str] = None,
		excluded_extensions: Set[str] = None,
		forced_inclusions: Set[str] = None
) -> List[tuple]:
	"""
Recursively build a list of (tree_line, is_directory, is_forced) tuples for the given path.

This function walks the directory tree starting at `current_path`,
but all exclusion decisions are based on paths relative to `project_root`.

Args:
current_path (Path): Current directory being processed.
project_root (Path): Absolute path to the project root (where .gitignore lives).
prefix (str): Indentation string for tree visualization.
hardcoded_names (Set[str]): Hardcoded names to exclude.
hardcoded_globs (List[str]): Hardcoded glob patterns to exclude.
gitignore_patterns (List[str]): Patterns from .gitignore.
excluded_extensions (Set[str]): File extensions to exclude.
forced_inclusions (Set[str]): Names to ALWAYS include.

Returns:
List[tuple]: List of (line_string, is_dir, is_forced) for tree rendering.
	"""
	if hardcoded_names is None:
		hardcoded_names = set()
	if hardcoded_globs is None:
		hardcoded_globs = []
	if gitignore_patterns is None:
		gitignore_patterns = []
	if excluded_extensions is None:
		excluded_extensions = set()
	if forced_inclusions is None:
		forced_inclusions = set()

	entries = []
	try:
		items = sorted(os.listdir(current_path))
	except (PermissionError, OSError) as e:
		logger.warning(f"Cannot read directory {current_path}: {e}")
		return entries

	# Separate directories and files
	dirs, files = [], []
	for item in items:
		full_path = current_path / item
		try:
			if full_path.is_symlink():
				# Skip symlinks to avoid infinite loops and unexpected behavior
				logger.debug(f"Skipping symlink: {full_path}")
				continue
			if full_path.is_dir():
				dirs.append(item)
			else:
				files.append(item)
		except OSError as e:
			logger.warning(f"OSError accessing {full_path}: {e}")
			continue

	all_items = dirs + files
	total_items = len(all_items)

	for idx, name in enumerate(all_items):
		full_path = current_path / name
		is_dir = name in dirs

		# Compute path relative to PROJECT ROOT (not target dir!)
		try:
			rel_path_obj = full_path.relative_to(project_root)
			relative_path_str = str(rel_path_obj).replace(os.sep, '/')
		except ValueError:
			# Should not happen if called correctly, but be safe
			logger.error(f"Path {full_path} is not under project root {project_root}")
			continue

		# Check if this item is a forced inclusion
		is_forced = is_forced_inclusion(name, forced_inclusions)

		# Check if this item should be excluded (forced inclusions bypass this)
		if should_exclude(
			relative_path_str,
			name,
			is_dir,
			hardcoded_names,
			hardcoded_globs,
			gitignore_patterns,
			excluded_extensions,
			forced_inclusions
		):
			continue

		# Determine tree connector
		is_last = (idx == total_items - 1)
		connector = "└── " if is_last else "├── "
		display_name = name + ("/" if is_dir else "")
		line = f"{prefix}{connector}{display_name}"
		entries.append((line, is_dir, is_forced))

		# Recurse into directories
		if is_dir:
			extension = "    " if is_last else "│   "
			entries.extend(
				build_tree_entries(
					full_path,
					project_root,
					prefix + extension,
					hardcoded_names,
					hardcoded_globs,
					gitignore_patterns,
					excluded_extensions,
					forced_inclusions
				)
			)

	return entries

def print_colored_console_tree(tree_entries: List[tuple], root_name: str):
	"""
Print a beautifully colored tree to the console using ANSI escape codes.

Uses different colors for:
- Root name (cyan)
- Directories (blue)
- Files (light gray)
- Forced inclusions (green)
- Tree connectors (yellow)

Args:
tree_entries (List[tuple]): Output from build_tree_entries.
root_name (str): Name of the root directory to display.
	"""
	print(f"\n{COLORS['root']}📁 {root_name}/{COLORS['reset']}")
	for line, is_dir, is_forced in tree_entries:
		# Split line to color only the filename part
		if "── " in line:
			prefix_part, name_part = line.split("── ", 1)
			# Use green for forced inclusions, otherwise normal colors
			if is_forced:
				color = COLORS['forced']
			else:
				color = COLORS['dir'] if is_dir else COLORS['file']
			connector_color = COLORS['connector']
			colored_line = f"{connector_color}{prefix_part}── {color}{name_part}{COLORS['reset']}"
			print(colored_line)
		else:
			print(line)
	print()  # Final newline for visual separation

# ----------------------------
# Main Execution Function
# ----------------------------

def main():
	"""
Main entry point of the script.

Steps:
1. Parse command-line arguments (target directory).
2. Resolve absolute paths for project root and target directory.
3. Load hardcoded exclusions (names, globs, extensions) and forced inclusions.
4. Load .gitignore from project root (if exists).
5. Build tree for target directory (with exclusions and forced inclusions).
6. Print colored tree to console.
7. Save markdown file in target directory.
	"""
	# Parse command-line arguments
	parser = argparse.ArgumentParser(
		description="Generate FILE-STRUCTURE.md for a target directory, using .gitignore from project root."
	)
	parser.add_argument(
		"target_dir",
		nargs="?",
		default=".",
		help="Target directory to analyze (default: current directory)"
	)
	args = parser.parse_args()

	# Resolve paths
	script_dir = Path(__file__).parent.resolve()  # Project root = where script lives
	target_dir = (Path(args.target_dir).resolve())

	if not target_dir.is_dir():
		logger.error(f"Target path is not a directory: {target_dir}")
		sys.exit(1)

	logger.info(f"Project root (for .gitignore): {script_dir}")
	logger.info(f"Target directory to analyze: {target_dir}")

	# Prepare exclusion sets and forced inclusions
	hardcoded_names = {normalize_name(name) for name in HARDCODED_EXCLUSIONS_NAMES}
	hardcoded_globs = HARDCODED_EXCLUSIONS_GLOBS.copy()
	excluded_extensions = HARDCODED_EXCLUDED_EXTENSIONS.copy()
	forced_inclusions = {normalize_name(name) for name in FORCED_INCLUSIONS}

	logger.debug(f"Hardcoded name exclusions: {hardcoded_names}")
	logger.debug(f"Hardcoded glob exclusions: {hardcoded_globs}")
	logger.debug(f"Hardcoded extension exclusions: {excluded_extensions}")
	logger.info(f"Forced inclusions (override all exclusions): {forced_inclusions}")

	# Load .gitignore from PROJECT ROOT (script directory)
	gitignore_path = script_dir / ".gitignore"
	gitignore_patterns = []
	if gitignore_path.exists():
		logger.info(f"Found .gitignore at project root: {gitignore_path}")
		gitignore_patterns = parse_gitignore(gitignore_path)
	else:
		logger.info("No .gitignore found in project root.")

	# Build tree entries for target directory
	logger.info("Building directory tree...")
	tree_entries = build_tree_entries(
		current_path=target_dir,
		project_root=script_dir,  # Critical: exclusions relative to project root
		hardcoded_names=hardcoded_names,
		hardcoded_globs=hardcoded_globs,
		gitignore_patterns=gitignore_patterns,
		excluded_extensions=excluded_extensions,
		forced_inclusions=forced_inclusions
	)

	# Print to console
	print_colored_console_tree(tree_entries, target_dir.name)

	# Write to FILE-STRUCTURE.md in TARGET directory
	output_path = target_dir / OUTPUT_FILENAME
	try:
		with open(output_path, 'w', encoding='utf-8') as f:
			f.write("# Project File Structure\n\n")
			f.write("```\n")
			f.write(f"{target_dir.name}/\n")
			for line, _, _ in tree_entries:
				f.write(line + "\n")
			f.write("```\n")
		logger.info(f"✅ Successfully wrote file structure to {output_path}")
	except Exception as e:
		logger.error(f"❌ Failed to write {output_path}: {e}")
		sys.exit(1)

# ----------------------------
# Script Entry Point
# ----------------------------

if __name__ == "__main__":
	"""
Guard clause for direct execution.
Handles KeyboardInterrupt and unexpected errors gracefully.
	"""
	try:
		main()
	except KeyboardInterrupt:
		logger.warning("Script interrupted by user (Ctrl+C).")
		sys.exit(1)
	except Exception as e:
		logger.critical(f"Unexpected error during execution: {e}")
		sys.exit(1)
