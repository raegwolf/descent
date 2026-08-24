#!/bin/sh
set -eu

destination=${1:?usage: fetch_sdl3.sh DESTINATION}
version=3.4.14
expected_sha=bae77509ccddcc7a443bb09730ab854c976e8f8bcf57b66d6bad6af2e17f38c2
archive_url="https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-${version}.dmg"
temporary=$(mktemp -d /tmp/descent-sdl3.XXXXXX)
mount_point="$temporary/mount"
mounted=0

cleanup() {
    if [ "$mounted" -eq 1 ]; then
        hdiutil detach "$mount_point" >/dev/null || true
    fi
    rm -rf "$temporary" || true
}
trap cleanup EXIT INT TERM

mkdir -p "$mount_point" "$destination"
curl -L --fail --silent --show-error -o "$temporary/SDL3.dmg" "$archive_url"
actual_sha=$(shasum -a 256 "$temporary/SDL3.dmg" | awk '{print $1}')
if [ "$actual_sha" != "$expected_sha" ]; then
    echo "SDL3 checksum mismatch: expected $expected_sha, got $actual_sha" >&2
    exit 1
fi

hdiutil attach -nobrowse -readonly -mountpoint "$mount_point" \
    "$temporary/SDL3.dmg" >/dev/null
mounted=1
ditto "$mount_point/SDL3.xcframework/macos-arm64_x86_64/SDL3.framework" \
    "$destination/SDL3.framework"
