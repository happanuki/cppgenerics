/*
 * ProgressBar.h
 *
 *  Created on: Jul 13, 2016
 *      Author: xama
 */

#pragma once

#include <iostream>
#include "Logger.h"

class ProgressBar
{

	std::string m_caption;

	size_t m_total;
	size_t m_initial;
	size_t m_current;

	std::ostream& m_stream;

	bool m_isOutput = false;

	const size_t m_barsCount = 50;
	int m_prevBars = -1;
	double m_perBarPercent;
	size_t m_prevNumPercentLen = 2;

	const char m_barMark = '=';


public:
	ProgressBar(std::string caption,std::ostream& stream = std::cout, size_t total=100, size_t initial=0);
	virtual ~ProgressBar();


	/*
	 * implicitly calls redraw()
	 */
	virtual void notify(size_t units=1);

	virtual void redraw();
	virtual void startOutput();
	virtual void stopOutput();

};

