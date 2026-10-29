#!/usr/bin/env bash
# Builds the macOS installer: one .pkg with a checkbox per format.
#
#   packaging/macos/build_pkg.sh <artefacts folder> <version> <output.pkg>
#   e.g. packaging/macos/build_pkg.sh build/Phasemap_artefacts/Release 1.0.0 Phasemap-macOS.pkg
#
# AU, VST3 and CLAP go to /Library/Audio/Plug-Ins, the app to /Applications.
# Files written by the macOS installer are not quarantined, so no xattr step is needed afterwards.
set -euo pipefail

artefacts="$1"; version="$2"; output="$3"
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

ids=()
add_component () {   # <id> <source bundle> <install folder> <title>
    local id="$1" source="$2" target="$3"
    [ -e "$source" ] || { echo "skipping $id: $source not found"; return; }
    mkdir -p "$work/root-$id"
    cp -R "$source" "$work/root-$id/"
    # keep pkgbuild from "relocating" the bundle to wherever an older copy happens to live
    pkgbuild --analyze --root "$work/root-$id" "$work/$id.plist"
    local i=0
    while plutil -replace "$i.BundleIsRelocatable" -bool NO "$work/$id.plist" 2>/dev/null; do i=$((i + 1)); done
    pkgbuild --root "$work/root-$id" --component-plist "$work/$id.plist" \
             --identifier "org.nullzone-audio.phasemap.$id" --version "$version" \
             --install-location "$target" "$work/$id.pkg"
    ids+=("$id")
}

add_component au         "$artefacts/AU/Phasemap.component"   "/Library/Audio/Plug-Ins/Components"
add_component vst3       "$artefacts/VST3/Phasemap.vst3"      "/Library/Audio/Plug-Ins/VST3"
add_component clap       "$artefacts/CLAP/Phasemap.clap"      "/Library/Audio/Plug-Ins/CLAP"
add_component standalone "$artefacts/Standalone/Phasemap.app" "/Applications"

title () { case "$1" in au) echo "Audio Unit (Logic, GarageBand, ...)";; vst3) echo "VST3";; clap) echo "CLAP";; standalone) echo "Standalone app";; esac; }

{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<installer-gui-script minSpecVersion="2">'
    echo '  <title>Phasemap</title>'
    echo '  <organization>org.nullzone-audio</organization>'
    echo '  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>'
    echo '  <domains enable_localSystem="true"/>'
    echo '  <welcome file="welcome.html"/>'
    echo '  <license file="license.txt"/>'
    echo '  <choices-outline>'
    for id in "${ids[@]}"; do echo "    <line choice=\"$id\"/>"; done
    echo '  </choices-outline>'
    for id in "${ids[@]}"; do
        echo "  <choice id=\"$id\" title=\"$(title "$id")\" visible=\"true\" start_selected=\"true\">"
        echo "    <pkg-ref id=\"org.nullzone-audio.phasemap.$id\"/>"
        echo '  </choice>'
        echo "  <pkg-ref id=\"org.nullzone-audio.phasemap.$id\" version=\"$version\">$id.pkg</pkg-ref>"
    done
    echo '</installer-gui-script>'
} > "$work/distribution.xml"

mkdir -p "$work/resources"
cp "$here/welcome.html" "$work/resources/"
cp "$here/../../LICENSE" "$work/resources/license.txt"

productbuild --distribution "$work/distribution.xml" --resources "$work/resources" \
             --package-path "$work" "$output"
echo "wrote $output"
