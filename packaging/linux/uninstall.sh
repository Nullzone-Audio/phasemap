#!/usr/bin/env bash
# Removes what install.sh installed. Use --system if you installed with --system.
set -euo pipefail
if [ "${1:-}" = "--system" ]; then
    paths=(/usr/local/lib/vst3/Phasemap.vst3 /usr/local/lib/clap/Phasemap.clap /usr/local/lib/lv2/Phasemap.lv2 /usr/local/bin/Phasemap)
else
    paths=("$HOME/.vst3/Phasemap.vst3" "$HOME/.clap/Phasemap.clap" "$HOME/.lv2/Phasemap.lv2" "$HOME/.local/bin/Phasemap")
fi
for p in "${paths[@]}"; do
    [ -e "$p" ] && rm -rf "$p" && echo "removed $p"
done
echo "Phasemap removed. Your settings stay in ~/.config/Nullzone Audio."
