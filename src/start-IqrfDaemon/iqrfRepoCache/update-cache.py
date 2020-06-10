#!/usr/bin/python3
# #############################################################################
# Author: 2019                                                                #
#         Rostislav Spinar <rostislav.spinar@iqrf.com>                        #
#         Roman Ondracek <roman.ondracek@iqrf.com>                            #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

import os
import json
import pathlib
import requests

server_url = 'server'
companies_url = 'companies'
manufacturers_url = 'manufacturers'
osdpa_url = 'osdpa'
products_url = 'products'
standards_url = 'standards'
packages_url = 'packages'

api_endpoint = 'https://repository.iqrfalliance.org/api/'

directories = [server_url, companies_url, manufacturers_url,
               osdpa_url, products_url, standards_url, packages_url]


def create_and_open_dir(directory, prefix=None):
    prefix = "" if prefix is None else (prefix + " ")
    if not os.path.exists(directory):
        os.mkdir(directory)
        print("Directory " + prefix + directory + " created.")
    else:
        print("Directory " + prefix + directory + " already exists.")

    os.chdir(directory)


def download_and_save_json(path):
    data = requests.get(api_endpoint + path).json()
    with open('data.json', 'w') as file:
        json.dump(data, file, sort_keys=True)
    return data


def update_packages(data):
    for json_object in data:
        pkg = str(json_object['packageID'])
        create_and_open_dir(pkg, "packages/pkgId")
        download_and_save_json(directory + '/' + pkg)
        update_package_handler(str(json_object['handlerUrl']))
        os.chdir('..')


def update_package_handler(handler_url):
    if handler_url.find('hex') != -1:
        file = open('handler.hex', 'w+b')
        file.write(requests.get(handler_url).content)
        file.close()


def update_standards(data):
    for json_object in data:
        std = str(json_object['standardID'])
        create_and_open_dir(std, "standards/stdId")
        data = download_and_save_json(directory + '/' + std)
        update_standard_versions(std, data['versions'])
        os.chdir('..')


def update_standard_versions(standard, versions):
    for ver in versions:
        ver = str(int(ver))
        create_and_open_dir(ver, "standards/stdId/ver")
        download_and_save_json(directory + '/' + standard + '/' + ver)
        os.chdir('..')


os.chdir(pathlib.Path(__file__).parent)

# loop via api/folders
for directory in directories:
    create_and_open_dir(directory)
    data = download_and_save_json(directory)

    # loop via std ids
    if directory == standards_url:
        update_standards(data)

    # loop via packages ids
    if directory == packages_url:
        update_packages(data)

    os.chdir('..')
