#!/usr/bin/env bash
# Installs Phasemap for the current user. Run it from the unpacked folder:  ./install.sh
# To install for all users instead:  sudo ./install.sh --system
set -euo pipefail
cd "$(dirname "$0")"

if [ "${1:-}" = "--system" ]; then
    vst3=/usr/local/lib/vst3; clap=/usr/local/lib/clap; lv2=/usr/local/lib/lv2; bin=/usr/local/bin
else
    vst3="$HOME/.vst3"; clap="$HOME/.clap"; lv2="$HOME/.lv2"; bin="$HOME/.local/bin"
fi

install_bundle () {   # <source> <folder>
    [ -e "$1" ] || return 0
    mkdir -p "$2"
    rm -rf "$2/$(basename "$1")"
    cp -R "$1" "$2/"
    echo "  $2/$(basename "$1")"
}

echo "Installing Phasemap:"
install_bundle Phasemap.vst3 "$vst3"
install_bundle Phasemap.clap "$clap"
install_bundle Phasemap.lv2  "$lv2"
install_bundle Phasemap      "$bin"
echo "Done. Rescan plugins in your DAW. To remove Phasemap, run ./uninstall.sh (with --system if you used it)."
