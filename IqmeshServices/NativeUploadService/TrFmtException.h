/*
 * Class of exceptions for clibtr file format parsers.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 * 
 * Adapted from clibcdc/include/CDCMessageParserException.h licenced under 
 * the Apache License, Version 2.0.
 */

#ifndef __TRFMTEXCEPTION_H__
#define __TRFMTEXCEPTION_H__

#include "TrException.h"


/**
 * Exception, which occurs during running of TR file format parsers.
 */
class TrFmtException : public TrException {
private:
	/* Identity string of this exception class. */
	std::string identity;

public:
	/**
	 * Constructs exception object.
	 * @param cause description of exception cause.
	 */
	TrFmtException(const char* cause);

	~TrFmtException() throw();
};

#define TR_THROW_FMT_EXCEPTION(file, line, pos, msg) { \
  std::ostringstream ostr; ostr << __FILE__ << " " << __LINE__ << " " << file << "[" << line << "/" << pos << "]: " << msg; \
  TrFmtException ex(ostr.str().c_str()); throw ex; }

#endif // __TRFMTEXCEPTION_H__