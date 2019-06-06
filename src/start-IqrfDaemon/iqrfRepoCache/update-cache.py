# #############################################################################
# Author: 2019                                                                #
#         Rostislav Spinar <rostislav.spinar@iqrf.com>                        #
#         Roman Ondracek <roman.ondracek@iqrf.com>                            #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

import os
import json
import urllib2

serverUrl = 'server'
companiesUrl = 'companies'
manufacturersUrl = 'manufacturers'
osdpaUrl = 'osdpa'
productsUrl = 'products'
standardsUrl = 'standards'
packagesUrl = 'packages'

dirsName = [serverUrl, companiesUrl, manufacturersUrl,
            osdpaUrl, productsUrl, standardsUrl, packagesUrl]

# loop via api/folders
for dir in dirsName:

    if not os.path.exists(dir):
        os.mkdir(dir)
        print("Directory ", dir,  " created.")
    else:
        print("Directory ", dir,  " already exists.")

    os.chdir(dir)

    data = json.load(urllib2.urlopen(
        'https://repository.iqrfalliance.org/api/' + dir))

    with open('data.json', 'w') as outfile:
        json.dump(data, outfile)

    # loop via std ids
    if dir == standardsUrl:

        for object in data:

            std = object['standardID']
            std = str(std)

            if not os.path.exists(std):
                os.mkdir(std)
                print("Directory standards/stdId ", std,  " created.")
            else:
                print("Directory standards/stdId ", std,  " already exists.")

            os.chdir(std)

            data = json.load(urllib2.urlopen(
                'https://repository.iqrfalliance.org/api/' + dir + '/' + std))

            with open('data.json', 'w') as outfile:
                json.dump(data, outfile)

	        # loop via ver of std ids
            for ver in data['versions']:

                ver = int(ver)
                ver = str(ver)

                if not os.path.exists(ver):
                    os.mkdir(ver)
                    print("Directory standards/stdId/ver ", ver,  " created.")
                else:
                    print("Directory standards/stdId/ver ",
                          ver,  " already exists.")

                os.chdir(ver)

                data = json.load(urllib2.urlopen(
                    'https://repository.iqrfalliance.org/api/' + dir + '/' + std + '/' + ver))

                with open('data.json', 'w') as outfile:
                    json.dump(data, outfile)

                os.chdir('..')

            os.chdir('..')

    # loop via packages ids
    if dir == packagesUrl:

        for object in data:

            pkg = object['packageID']
            pkg = str(pkg)

            if not os.path.exists(pkg):
                os.mkdir(pkg)
                print("Directory packages/pkgId ", pkg,  " created.")
            else:
                print("Directory packages/pkgId ", pkg,  " already exists.")

            os.chdir(pkg)

            data = json.load(urllib2.urlopen(
                'https://repository.iqrfalliance.org/api/' + dir + '/' + pkg))

            with open('data.json', 'w') as outfile:
            	json.dump(data, outfile)

	    hdlUrl = object['handlerUrl']
	    hdlUrl = str(hdlUrl)
	    if (hdlUrl.find('hex') != -1):

		    hex = urllib2.urlopen(hdlUrl)

		    f = open('handler.hex', 'w+b')
		    f.write(hex.read())
		    f.close()

	    os.chdir('..')

    os.chdir('..')
