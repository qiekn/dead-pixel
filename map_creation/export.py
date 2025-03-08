import sys
from PIL import Image


DEFAULT_PATH = "map.png"
OUTPUT_FILEPATH = "map.txt"

BLACK = (0, 0, 0, 255)
YELLOW = (255, 255, 0, 255)


def main() -> None:
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH

    image = Image.open(path)
    map = [['0' for _ in range(image.width)] for _ in range(image.height)]

    # Read image file
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            if pixels[x, y] == BLACK:
                continue
            if pixels[x, y] == YELLOW:
                map[y][x] = '2'
                continue
            map[y][x] = '1'

    # Write output file
    with open(OUTPUT_FILEPATH, "w") as f:
        for y in range(image.height):
            f.write(", ".join(map[y]) + '\n')


if __name__ == "__main__":
    main()
