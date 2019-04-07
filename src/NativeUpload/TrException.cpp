/*
 * Base class of exceptions for clibtr.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 * 
 * Adapted from clibcdc/src/CDCImplException.cpp licenced under 
 * the Apache License, Version 2.0.
 */


#include <TrException.h>


void TrException::createDescription() {
    descr.clear();
    descr.append(identity);
    descr.append(": ");
    descr.append(cause);
}

TrException::TrException(const char* cause) {
    this->identity = "TrException";
    this->cause = cause;
    createDescription();
}

TrException::~TrException() throw() {
    identity.clear();
    cause.clear();
    descr.clear();
}

/*
 * Returns the cause of this exception.
 */
const char* TrException::what() const throw() {
    return cause.c_str();
}

/*
 * Returns complete description of this exception.
 */
const char* TrException::getDescr() {
    return descr.c_str();
}
