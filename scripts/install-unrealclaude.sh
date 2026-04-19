#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# UnrealClaude build + install helper (macOS, Apple Silicon)
# ----------------------------------------------------------------------------
# Usage:
#   ./install-unrealclaude.sh                    # install existing BuiltPlugin into TARGET_PROJECT
#   ./install-unrealclaude.sh --build            # build first, then install to TARGET_PROJECT
#   ./install-unrealclaude.sh --engine           # install engine-wide (all projects see it)
#   ./install-unrealclaude.sh --build --engine   # build, then install engine-wide
#   Flags: -b = --build, -e = --engine
# ============================================================================

# --- Config --------------------------------------------------------------
UE_ROOT="/Volumes/MySSD/EpicGames/UE_5.7"
REPO_ROOT="/Users/daniel/Documents/Coding/Games/UnrealClaude"
TARGET_PROJECT=""  # <-- set this to your .uproject's parent directory, e.g. /Users/daniel/Documents/Unreal Projects/MyGame

# Marketplace plugin that breaks universal builds on Apple Silicon. Left empty
# to skip; set to a folder name under $UE_ROOT/Engine/Plugins/Marketplace to
# disable it during the build.
BROKEN_MARKETPLACE_PLUGIN="Untitledca7e22d4545aV1"

# Derived paths
UPLUGIN="$REPO_ROOT/UnrealClaude/UnrealClaude.uplugin"
BUILD_OUTPUT="$REPO_ROOT/BuiltPlugin"
RUN_UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
MARKETPLACE_DIR="$UE_ROOT/Engine/Plugins/Marketplace"

# --- Args ----------------------------------------------------------------
DO_BUILD=0
INSTALL_ENGINE=0
for arg in "$@"; do
  case "$arg" in
    --build|-b)  DO_BUILD=1 ;;
    --engine|-e) INSTALL_ENGINE=1 ;;
    -h|--help)
      sed -n '3,11p' "$0"
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      echo "Run with --help for usage." >&2
      exit 1
      ;;
  esac
done

# --- Sanity checks -------------------------------------------------------
if [[ $INSTALL_ENGINE -eq 1 ]]; then
  if [[ ! -d "$UE_ROOT/Engine/Plugins/Marketplace" ]]; then
    echo "ERROR: Engine Marketplace plugins dir not found: $UE_ROOT/Engine/Plugins/Marketplace" >&2
    echo "Is the SSD mounted?" >&2
    exit 1
  fi
else
  if [[ -z "$TARGET_PROJECT" ]]; then
    echo "ERROR: TARGET_PROJECT is not set. Edit the script and point it at your UE project directory," >&2
    echo "       or pass --engine to install engine-wide instead." >&2
    exit 1
  fi
  if [[ ! -d "$TARGET_PROJECT" ]]; then
    echo "ERROR: TARGET_PROJECT does not exist: $TARGET_PROJECT" >&2
    exit 1
  fi
fi

# --- Build (optional) ----------------------------------------------------
if [[ $DO_BUILD -eq 1 ]]; then
  if [[ ! -x "$RUN_UAT" ]]; then
    echo "ERROR: RunUAT.sh not found or not executable at $RUN_UAT" >&2
    echo "Is the SSD mounted?" >&2
    exit 1
  fi
  if [[ ! -f "$UPLUGIN" ]]; then
    echo "ERROR: .uplugin not found: $UPLUGIN" >&2
    exit 1
  fi

  # Disable known-broken Marketplace plugin for the duration of the build
  DISABLED_PATH=""
  if [[ -n "$BROKEN_MARKETPLACE_PLUGIN" && -d "$MARKETPLACE_DIR/$BROKEN_MARKETPLACE_PLUGIN" ]]; then
    DISABLED_PATH="$MARKETPLACE_DIR/${BROKEN_MARKETPLACE_PLUGIN}.disabled"
    echo ">>> Temporarily disabling Marketplace plugin: $BROKEN_MARKETPLACE_PLUGIN"
    sudo mv "$MARKETPLACE_DIR/$BROKEN_MARKETPLACE_PLUGIN" "$DISABLED_PATH"
    # Re-enable on exit, even if build fails
    trap 'if [[ -n "${DISABLED_PATH:-}" && -d "$DISABLED_PATH" ]]; then echo ">>> Restoring $BROKEN_MARKETPLACE_PLUGIN"; sudo mv "$DISABLED_PATH" "$MARKETPLACE_DIR/$BROKEN_MARKETPLACE_PLUGIN"; fi' EXIT
  fi

  echo ">>> Building plugin..."
  rm -rf "$BUILD_OUTPUT"
  "$RUN_UAT" BuildPlugin \
    -Plugin="$UPLUGIN" \
    -Package="$BUILD_OUTPUT" \
    -TargetPlatforms=Mac \
    -Rocket
  echo ">>> Build finished."
fi

# --- Install -------------------------------------------------------------
if [[ ! -d "$BUILD_OUTPUT" ]]; then
  echo "ERROR: No BuiltPlugin found at $BUILD_OUTPUT. Run with --build first." >&2
  exit 1
fi

if [[ $INSTALL_ENGINE -eq 1 ]]; then
  DEST="$UE_ROOT/Engine/Plugins/Marketplace/UnrealClaude"
  SUDO="sudo"
  echo ">>> Installing plugin engine-wide to: $DEST (needs sudo)"
else
  DEST="$TARGET_PROJECT/Plugins/UnrealClaude"
  SUDO=""
  echo ">>> Installing plugin to project: $DEST"
  mkdir -p "$TARGET_PROJECT/Plugins"
fi

$SUDO rm -rf "$DEST"
$SUDO cp -R "$BUILD_OUTPUT" "$DEST"

# --- MCP bridge npm deps -------------------------------------------------
MCP_DIR="$DEST/Resources/mcp-bridge"
if [[ -d "$MCP_DIR" ]]; then
  echo ">>> Installing MCP bridge npm deps..."
  ( cd "$MCP_DIR" && $SUDO npm install )
else
  echo "WARN: MCP bridge directory missing at $MCP_DIR — skipping npm install." >&2
fi

echo ">>> Done. Launch the editor and open Tools > Claude Assistant."
