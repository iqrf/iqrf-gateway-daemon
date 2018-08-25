/*
 * Base class of exceptions for clibtr.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 * 
 * Adapted from clibcdc/include/CDCImplException.h licenced under 
 * the Apache License, Version 2.0.
 */

#ifndef __TREXCEPTION_H__
#define __TREXCEPTION_H__

#include <exception>
#include <sstream>
#include <string>


/**
 * Base class of exception in library clibtr
 */
class TrException : public std::exception {
private:
	/* Identity string of this exception class. */
	std::string identity;

	/* Complete desription of exception. */
	std::string descr;

	/* Creates desription. */
	void createDescription();

protected:
	/* Cause of exception. */
	std::string cause;

public:
	/**
	 * Constructs exception object.
	 * @param cause description of exception cause.
	 */
	TrException(const char* cause);

	~TrException() throw();

	/**
	 * Returns description of exception cause.
	 * @return description of this exception cause.
	 */
	const char* what() const throw();

	/**
	 * Returns complete description of this exception.
	 * @return complete information of this exception.
	 */
	const char* getDescr();
};

#define TR_THROW_EXCEPTION(extype, msg) { \
  std::ostringstream ostr; ostr << __FILE__ << " " << __LINE__ << msg; \
  extype ex(ostr.str().c_str()); throw ex; }

#endif // __TREXCEPTION_H__
