#!/bin/bash
set -euo pipefail

# Release script for BerserkAudio
# Usage: ./release.sh <version>
# Example: ./release.sh 2.1.0

VERSION="$1"

if [[ -z "$VERSION" ]]; then
    echo "Usage: ./release.sh <version>"
    echo "Example: ./release.sh 2.1.0"
    exit 1
fi

# Validate semver format
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in semver format (e.g. 2.1.0)"
    exit 1
fi

# Check for uncommitted changes
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Error: You have uncommitted changes. Commit or stash them first."
    exit 1
fi

# Show what we're about to do
CURRENT_VERSION=$(jq -r '.version' plugin.json)
echo ""
echo "  BerserkAudio Release"
echo "  ───────────────────"
echo "  Current version:  $CURRENT_VERSION"
echo "  New version:      $VERSION"
echo ""
echo "  This will:"
echo "    1. Update plugin.json       ($CURRENT_VERSION → $VERSION)"
echo "    2. Update CMakeLists.txt    (VERSION $VERSION)"
echo "    3. Commit: \"Bump version to $VERSION\""
echo "    4. Tag:    v$VERSION  (triggers build for VCV Rack + MetaModule)"
echo "    5. Push commit + tag"
echo ""
read -p "  Proceed? [y/N] " confirm
if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "Aborted."
    exit 0
fi

# 1. Update plugin.json
jq --arg v "$VERSION" '.version = $v' plugin.json > plugin.json.tmp && mv plugin.json.tmp plugin.json
echo "Updated plugin.json → $VERSION"

# 2. Update CMakeLists.txt
sed -i.bak -E "s/(^    VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1$VERSION/" CMakeLists.txt && rm CMakeLists.txt.bak
if ! grep -q "^    VERSION $VERSION$" CMakeLists.txt; then
    echo "Error: CMakeLists.txt VERSION line did not update to $VERSION (no match for X.Y.Z pattern)"
    exit 1
fi
echo "Updated CMakeLists.txt → VERSION $VERSION"

# 3. Commit
git add plugin.json CMakeLists.txt
git commit -m "Bump version to $VERSION"
echo "Committed."

# 4. Tag
git tag "v$VERSION"
echo "Tagged v$VERSION"

# 5. Push
git push && git push --tags
echo ""
echo "Done! Build running at:"
echo "  https://github.com/NicolasMurphy/BerserkAudio/actions/workflows/build-plugin.yml"
