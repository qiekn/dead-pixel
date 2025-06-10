## Script setup
### I would recommend setting up a [virtual environment](https://www.freecodecamp.org/news/how-to-setup-virtual-environments-in-python/) before installing this project's requirements</sub>
  
Install the project requirements  
```
pip install -r requirements.txt
```

## Running the script
```
python3 export.py <path_to_png_file>
```
If the `<path_to_png_file>` is left empty then the default path of `map.png` will be used  
All the script does is read the pixels of the image and write the data to a txt file named `map.txt`  
Currently every image in the image that is not RGBA (0, 0, 0, 255) is set to be a 1 in the map data format representing a wall  
The map data format is just integers separated by commas
