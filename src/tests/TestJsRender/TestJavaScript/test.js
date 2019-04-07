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

