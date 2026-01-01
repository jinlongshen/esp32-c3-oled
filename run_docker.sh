#!/bin/bash

# ------------------------------------------------------------
# 1. Configuration
# ------------------------------------------------------------
IMAGE_NAME="ghcr.io/jinlongshen/esp-idf-c3-freetype"
TAG="latest"

echo "🔍 Checking for updates at $IMAGE_NAME:$TAG..."

# ------------------------------------------------------------
# 2. Fetch remote digest (without pulling full image)
# ------------------------------------------------------------
REMOTE_SHA=$(curl -s "https://ghcr.io/v2/${IMAGE_NAME#ghcr.io/}/manifests/${TAG}" \
            -H "Accept: application/vnd.oci.image.index.v1+json" \
            | grep -oP '(?<=\"digest\": \")[^\"]*' | head -n 1)

# ------------------------------------------------------------
# 3. Get local digest
# ------------------------------------------------------------
LOCAL_SHA=$(docker inspect --format='{{index .RepoDigests 0}}' \
            "${IMAGE_NAME}:${TAG}" 2>/dev/null | cut -d'@' -f2)

# ------------------------------------------------------------
# 4. Compare and pull if needed
# ------------------------------------------------------------
if [ -z "$LOCAL_SHA" ] || [ "$REMOTE_SHA" != "$LOCAL_SHA" ]; then
    echo "🚀 New version or first run detected! Updating image..."
    docker pull "${IMAGE_NAME}:${TAG}"
else
    echo "✅ Image is already up to date."
fi

# ------------------------------------------------------------
# 5. Run container AS ROOT (required for ESP-IDF)
# ------------------------------------------------------------
echo "💻 Starting container as ROOT..."

docker run --rm -it \
  --privileged \
  -v /dev/bus/usb:/dev/bus/usb \
  -v "$PWD":/project \
  -w /project \
  --device=/dev/ttyACM0 \
  "${IMAGE_NAME}:${TAG}" \
  bash

