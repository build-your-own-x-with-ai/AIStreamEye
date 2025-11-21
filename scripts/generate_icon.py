#!/usr/bin/env python3
"""
Generate AIStreamEye application icon
Creates a simple icon with the app name and a video symbol
"""

try:
    from PIL import Image, ImageDraw, ImageFont
    import os
    
    def create_icon():
        """Create a simple icon for AIStreamEye"""
        
        # Create base image (256x256 for high quality)
        size = 256
        img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        # Background gradient (dark blue to lighter blue)
        for y in range(size):
            alpha = int(255 * (1 - y / size * 0.3))
            color = (30 + y // 4, 60 + y // 3, 120 + y // 2, alpha)
            draw.rectangle([(0, y), (size, y + 1)], fill=color)
        
        # Draw rounded rectangle border
        border_width = 8
        border_color = (100, 180, 255, 255)
        draw.rounded_rectangle(
            [(border_width, border_width), (size - border_width, size - border_width)],
            radius=20,
            outline=border_color,
            width=border_width
        )
        
        # Draw video play symbol (triangle)
        play_size = 80
        play_x = size // 2 - 10
        play_y = size // 2 - 40
        play_points = [
            (play_x, play_y),
            (play_x + play_size, play_y + play_size // 2),
            (play_x, play_y + play_size)
        ]
        draw.polygon(play_points, fill=(255, 255, 255, 230))
        
        # Draw "AI" text
        try:
            # Try to use a nice font
            font_large = ImageFont.truetype("arial.ttf", 48)
            font_small = ImageFont.truetype("arial.ttf", 24)
        except:
            # Fallback to default font
            font_large = ImageFont.load_default()
            font_small = ImageFont.load_default()
        
        # Draw "AI" at top
        text_ai = "AI"
        bbox = draw.textbbox((0, 0), text_ai, font=font_large)
        text_width = bbox[2] - bbox[0]
        text_x = (size - text_width) // 2
        draw.text((text_x, 20), text_ai, fill=(255, 255, 255, 255), font=font_large)
        
        # Draw "StreamEye" at bottom
        text_stream = "StreamEye"
        bbox = draw.textbbox((0, 0), text_stream, font=font_small)
        text_width = bbox[2] - bbox[0]
        text_x = (size - text_width) // 2
        draw.text((text_x, size - 50), text_stream, fill=(255, 255, 255, 255), font=font_small)
        
        return img
    
    # Create output directory
    os.makedirs('resources', exist_ok=True)
    
    # Generate icon
    print("🎨 Generating AIStreamEye icon...")
    icon = create_icon()
    
    # Save as PNG (high quality)
    png_path = 'resources/icon.png'
    icon.save(png_path, 'PNG')
    print(f"✅ Saved PNG: {png_path}")
    
    # Save as ICO (Windows) - multiple sizes
    ico_path = 'resources/icon.ico'
    icon_sizes = [(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)]
    icons = []
    for size in icon_sizes:
        resized = icon.resize(size, Image.Resampling.LANCZOS)
        icons.append(resized)
    
    icons[0].save(ico_path, format='ICO', sizes=icon_sizes)
    print(f"✅ Saved ICO: {ico_path}")
    
    # Save as ICNS (macOS) if possible
    try:
        # macOS iconset requires specific sizes
        iconset_dir = 'resources/icon.iconset'
        os.makedirs(iconset_dir, exist_ok=True)
        
        icns_sizes = [
            (16, 'icon_16x16.png'),
            (32, 'icon_16x16@2x.png'),
            (32, 'icon_32x32.png'),
            (64, 'icon_32x32@2x.png'),
            (128, 'icon_128x128.png'),
            (256, 'icon_128x128@2x.png'),
            (256, 'icon_256x256.png'),
            (512, 'icon_256x256@2x.png'),
            (512, 'icon_512x512.png'),
            (1024, 'icon_512x512@2x.png'),
        ]
        
        for size, filename in icns_sizes:
            resized = icon.resize((size, size), Image.Resampling.LANCZOS)
            resized.save(os.path.join(iconset_dir, filename), 'PNG')
        
        print(f"✅ Created iconset: {iconset_dir}")
        print("   To create .icns on macOS, run:")
        print(f"   iconutil -c icns {iconset_dir}")
        
    except Exception as e:
        print(f"⚠️  Could not create iconset: {e}")
    
    print("\n✨ Icon generation complete!")
    print("\nNext steps:")
    print("1. Review the generated icon: resources/icon.png")
    print("2. Rebuild the project to include the icon")
    print("3. On Windows: The .ico will be embedded automatically")
    print("4. On macOS: Run 'iconutil -c icns resources/icon.iconset' to create .icns")

except ImportError:
    print("❌ PIL (Pillow) is required to generate icons")
    print("Install it with: pip install Pillow")
    print("\nAlternatively, you can:")
    print("1. Create your own icon.ico file (256x256 recommended)")
    print("2. Place it in the resources/ directory")
    print("3. Rebuild the project")
    exit(1)

if __name__ == '__main__':
    create_icon()
