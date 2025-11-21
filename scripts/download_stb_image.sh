#!/bin/bash

echo "================================================"
echo "  Downloading stb_image.h"
echo "================================================"
echo ""

# Create directory
mkdir -p external/stb

# Download stb_image.h
echo "📥 Downloading stb_image.h from GitHub..."
curl -L -o external/stb/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

if [ $? -eq 0 ]; then
    echo "✅ Successfully downloaded stb_image.h"
    echo ""
    echo "File saved to: external/stb/stb_image.h"
else
    echo "❌ Failed to download stb_image.h"
    echo ""
    echo "Please download manually from:"
    echo "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    echo ""
    echo "And save it to: external/stb/stb_image.h"
    exit 1
fi

echo ""
echo "================================================"
echo "✨ Download complete!"
echo "================================================"
