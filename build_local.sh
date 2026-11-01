#!/bin/bash
# Builds IMSProg_programmer, IMSProg_editor and IMSProg_database_update
# without root permissions. Nothing is installed and the build/ directories
# are kept, so subsequent runs are incremental.
# The binaries are located in <project>/build/ after building.

set -e

[[ "$OSTYPE" == "darwin"* ]] && export C_INCLUDE_PATH=/usr/local/opt/libusb/include

if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
else
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
fi

cd "$(dirname "$0")"

for project in IMSProg_programmer IMSProg_editor IMSProg_database_update
do
    echo "=== Building $project ==="
    cmake -S "$project" -B "$project/build"
    cmake --build "$project/build" --parallel "$JOBS"
done

echo
echo "Build finished:"
echo "  IMSProg_programmer/build/IMSProg"
echo "  IMSProg_editor/build/IMSProg_editor"
echo "  IMSProg_database_update/build/IMSProg_database_update"

if [[ "$OSTYPE" == "darwin"* ]]; then
    USER_DB="$HOME/Library/Application Support/imsprog/IMSProg.Dat"
else
    USER_DB="${XDG_DATA_HOME:-$HOME/.local/share}/imsprog/IMSProg.Dat"
fi
if [ ! -f "$USER_DB" ]; then
    echo
    echo "Note: no chip database found at $USER_DB"
    echo "Run IMSProg_database_update or copy IMSProg_programmer/database/IMSProg.Dat there."
fi
