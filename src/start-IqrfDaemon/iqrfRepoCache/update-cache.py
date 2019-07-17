#!/usr/bin/python3
# #############################################################################
# Author: 2019                                                                #
#         Rostislav Spinar <rostislav.spinar@iqrf.com>                        #
#         Roman Ondracek <roman.ondracek@iqrf.com>                            #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

import os
import json
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

# loop via api/folders
for directory in directories:

    if not os.path.exists(directory):
        os.mkdir(directory)
        print("Directory " + directory + " created.")
    else:
        print("Directory " + directory + " already exists.")

    os.chdir(directory)

    data = requests.get(api_endpoint + directory).json()

    with open('data.json', 'w') as output_file:
        json.dump(data, output_file, sort_keys=True)

    # loop via std ids
    if directory == standards_url:

        for json_object in data:

            std = str(json_object['standardID'])

            if not os.path.exists(std):
                os.mkdir(std)
                print("Directory standards/stdId " + std + " created.")
            else:
                print("Directory standards/stdId " + std + " already exists.")

            os.chdir(std)

            data = requests.get(api_endpoint + directory + '/' + std).json()

            with open('data.json', 'w') as output_file:
                json.dump(data, output_file, sort_keys=True)

            # loop via ver of std ids
            for ver in data['versions']:

                ver = str(int(ver))

                if not os.path.exists(ver):
                    os.mkdir(ver)
                    print("Directory standards/stdId/ver " + ver + " created.")
                else:
                    print("Directory standards/stdId/ver " + ver +
                          " already exists.")

                os.chdir(ver)

                data = requests.get(api_endpoint + directory + '/' + std +
                                    '/' + ver).json()

                with open('data.json', 'w') as output_file:
                    json.dump(data, output_file, sort_keys=True)

                os.chdir('..')

            os.chdir('..')

    # loop via packages ids
    if directory == packages_url:

        for json_object in data:

            pkg = str(json_object['packageID'])

            if not os.path.exists(pkg):
                os.mkdir(pkg)
                print("Directory packages/pkgId " + pkg + " created.")
            else:
                print("Directory packages/pkgId " + pkg + " already exists.")

            os.chdir(pkg)

            data = requests.get(api_endpoint + directory + '/' + pkg).json()

            with open('data.json', 'w') as output_file:
                json.dump(data, output_file, sort_keys=True)

            handler_url = str(json_object['handlerUrl'])
            if handler_url.find('hex') != -1:
                file = open('handler.hex', 'w+b')
                file.write(requests.get(handler_url).content)
                file.close()
            os.chdir('..')

    os.chdir('..')
