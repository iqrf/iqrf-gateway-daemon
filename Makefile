#!/bin/bash

# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
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

clean:
	rm -rf cppcheck-report.xml cppcheck-html-report

cppcheck: clean
	cppcheck --enable=all --inconclusive --std=c++17 -I src/include src/

cppcheck-xml: clean
	cppcheck --enable=all --inconclusive --std=c++17 --xml --xml-version=2 -I src/include src/ 2> cppcheck-report.xml

cppcheck-html: cppcheck-xml
	cppcheck-htmlreport --file=cppcheck-report.xml --report-dir=cppcheck-html-report --source-dir=.
