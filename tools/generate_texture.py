"""
Generate a simple checkerboard texture for Step03_Texture
Requires: pip install Pillow
"""
from PIL import Image

def create_checkerboard(width=256, height=256, square_size=32):
    img = Image.new('RGB', (width, height))
    pixels = img.load()
    
    for y in range(height):
        for x in range(width):
            square_x = x // square_size
            square_y = y // square_size
            if (square_x + square_y) % 2 == 0:
                pixels[x, y] = (255, 100, 100)  # Red
            else:
                pixels[x, y] = (100, 100, 255)  # Blue
    
    return img

if __name__ == '__main__':
    img = create_checkerboard()
    img.save('steps/Step03_Texture/assets/texture.png')
    print('Created texture.png')
