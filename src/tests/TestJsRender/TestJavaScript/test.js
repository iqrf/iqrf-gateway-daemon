/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

