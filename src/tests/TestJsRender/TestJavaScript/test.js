/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
"use strict";

// JavaScript 'namespace' helper routine
var globalObject = this;

/* Function: namespace
Declares a new namespace.

Parameters:
  name - string: Namespace name. Use dots for multilevel namespace.
*/
var namespace = function (name) {
    var tokens = name.split('.');
    var object = globalObject;
    while (tokens.length > 0) {
        var token = tokens.shift();
        object = object[token] = object[token] || {};
    }
    return object;
};


namespace('test');

test.convertUpperCase = function ( inp )
{
  if ( inp === undefined ) {
    throw new Error( 'inp is undefined');
  }
  if ( typeof inp !== 'string' ) {
    throw new Error( 'inp is defined but type is ' + typeof rdata );
  }
  var result =
  {
    out: inp.toUpperCase()
  };
  return result;
};

