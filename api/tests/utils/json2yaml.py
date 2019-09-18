#!c:\python37\python.exe

# Copyright 2015 David R. Bild
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License

"""
Usage:
    json2yaml (--version|--help)
    json2yaml [<json_file>] [<yaml_file>]

Arguments:
    <json_file>    The input file containing the JSON to convert. If not
                   specified, reads from stdin.
    <yaml_file>    The output file to which to write the converted YAML. If 
                   not specified, writes to stdout.
"""

import sys, os
import collections
import json, pyaml
import docopt

__version__ = "1.1.1"

def convert(json_file, yaml_file):
    loaded_json = json.load(json_file, object_pairs_hook=collections.OrderedDict)
    pyaml.dump(loaded_json, yaml_file, safe=True)

if __name__ == '__main__':
    args = docopt.docopt(
        __doc__,
        version="version "+__version__
    )

    json_file = sys.stdin
    if args.get('<json_file>'):
        json_file = open(args.get('<json_file>'), 'r')

    with json_file:
        yaml_file = sys.stdout
        if args.get('<yaml_file>'):
            yaml_file = open(args.get('<yaml_file>'), 'w')
        with yaml_file:
            convert(json_file, yaml_file)
