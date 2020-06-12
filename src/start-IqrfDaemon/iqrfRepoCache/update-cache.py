#!/usr/bin/python3
# #############################################################################
# Author: 2019                                                                #
#         Rostislav Spinar <rostislav.spinar@iqrf.com>                        #
#         Roman Ondracek <roman.ondracek@iqrf.com>                            #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

from io import BytesIO
from zipfile import ZipFile

import os
import pathlib
import requests

api_endpoint = 'https://repository.iqrfalliance.org/api/'


def download_zip():
    """
    Downloads ZIP archive from IQRF Repository
    """
    response = requests.get(api_endpoint + 'zip').content
    zip_file = ZipFile(BytesIO(response))
    zip_file.extractall()


def main():
    os.chdir(pathlib.Path(__file__).parent)
    download_zip()


if __name__ == '__main__':
    main()