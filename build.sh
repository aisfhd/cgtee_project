#!/usr/bin/env bash

BUILD_MODE="debug"
COMPILE=$true
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

function cleanup_buildf(){
    if [ "$BUILD_MODE" == "debug" ]; then
    rm -rf ./build_debug
    elif [ "$BUILD_MODE" == "release" ]; then
    rm -rf ./build_release
    fi
}

function run_debug_build(){
    ./bash_scripts/debug_compile.sh
}

function run_release_build(){
    ./bash_scripts/release_compile.sh
}

while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
        --debug|-d)
        BUILD_MODE="debug"
        shift
        ;;
        --release|-r)
        BUILD_MODE="release"
        shift
        ;;
        --cleanup|-c)
        cleanup_buildf
        shift
        ;;
        --help|-h)
        echo "Usage: $0 [OPTION]"
        echo ""
        echo "  -d, --debug    Run debug build scripts(default)."
        echo "  -r, --release  Run release build scripts."
        echo "  -h, --help     Display this help message."
        echo "  -c, --cleanup  Cleanup build directories and build."
        exit 0
        ;;
        *)
        echo "Unknown option: $key"
        echo "Try '$0 --help' for more information."
        exit 1
        ;;
    esac
done

echo "--- Script Execution ---"
if [ "$BUILD_MODE" == "debug" ]; then
    run_debug_build
elif [ "$BUILD_MODE" == "release" ]; then
    run_release_build
else
    echo "unknown build mode: $BUILD_MODE"
    exit 1
fi
