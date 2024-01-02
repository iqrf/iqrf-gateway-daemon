#!/usr/bin/python3

# Copyright 2015-2024 IQRF Tech s.r.o.
# Copyright 2019-2024 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
    zip_file.extractall('cache')


def main():
    os.chdir(pathlib.Path(__file__).parent)
    download_zip()


if __name__ == '__main__':
    main()