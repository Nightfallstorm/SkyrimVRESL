#!/usr/bin/env bash
# Updates cmake/ports/SkyrimVRESL/portfile.cmake with the correct REF and SHA512
# for the given tag (defaults to the latest git tag).
#
# Usage:
#   ./cmake/ports/update-portfile.sh            # uses latest tag
#   ./cmake/ports/update-portfile.sh v1.3.0     # uses specific tag

set -euo pipefail

REPO="Nightfallstorm/SkyrimVRESL"
PORTFILE="cmake/ports/SkyrimVRESL/portfile.cmake"
TAG="${1:-$(git describe --tags --abbrev=0)}"

echo "Resolving tag: ${TAG}"
COMMIT=$(git rev-parse "${TAG}")
echo "Commit: ${COMMIT}"

# Download the GitHub tarball and compute SHA512 (retry up to 3 times)
URL="https://github.com/${REPO}/archive/${COMMIT}.tar.gz"
for attempt in 1 2 3; do
    echo "Downloading tarball (attempt ${attempt}): ${URL}"
    SHA512=$(curl -fsSL "${URL}" | sha512sum | awk '{print $1}') && break
    if [[ ${attempt} -eq 3 ]]; then
        echo "ERROR: Failed to download tarball after 3 attempts" >&2
        exit 1
    fi
    sleep 5
done
echo "SHA512: ${SHA512}"

# Update portfile in-place
sed -i "s|REF .*|REF ${COMMIT}|" "${PORTFILE}"
sed -i "s|SHA512 .*|SHA512 ${SHA512}|" "${PORTFILE}"

echo "Updated ${PORTFILE}"
