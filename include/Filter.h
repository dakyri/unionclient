/*
 * Filter.h
 *
 *  Created on: Apr 14, 2014
 *      Author: dak
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "CommonTypes.h"

class IFilter {
public:
	virtual ~IFilter();
	virtual std::string ToXMLString() const =0;
};

class Filter: public IFilter {
public:
	Filter();
	virtual ~Filter();

	virtual std::string ToXMLString() const override;
};

#endif /* FILTER_H_ */
