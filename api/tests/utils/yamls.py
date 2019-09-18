# It creates YAML from JSON files in the current directory.
# It deletes JSON files.

import glob
from pathlib import Path
import os

def filebrowser():
    return [f for f in glob.glob('*.json')]

if __name__ == '__main__':

    files = filebrowser()

    for fileJson in files:
        fileYaml = Path(fileJson).stem + '.yaml'

        os.system('python -u json2yaml.py ' + fileJson + ' ' + fileYaml)
        os.remove(fileJson)
        
        print(fileYaml + ' created, ' + fileJson + ' deleted.')
