/*
 * Class of exceptions for clibtr file format parsers.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 * 
 * Adapted from clibcdc/include/CDCMessageParserException.h licenced under 
 * the Apache License, Version 2.0.
 */

#include <TrFmtException.h>


TrFmtException::TrFmtException(const char* cause):TrException(cause) {
	this->identity = "TrFmtException";
}

TrFmtException::~TrFmtException() throw() {
    identity.clear();
}
