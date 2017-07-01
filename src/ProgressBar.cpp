/*
 * ProgressBar.cpp
 *
 *  Created on: Jul 13, 2016
 *      Author: xama
 */

#include "cppgenerics/ProgressBar.h"

ProgressBar::ProgressBar(std::string caption,std::ostream& stream, size_t total, size_t initial):
	m_caption(caption),
	m_total(total),
	m_initial(initial),
	m_current(initial),
	m_stream(stream)
{
	m_perBarPercent = 100 / m_barsCount;
}

ProgressBar::~ProgressBar()
{
	ProgressBar::stopOutput();
}

void ProgressBar::notify(size_t units)
{
	m_current+=units;
	redraw();
}
void ProgressBar::redraw()
{
	if (!m_isOutput){
		WARNSTDOUT("PROGRESSBAR redraw(), with m_isOutput == false");
		return;
	}

	double percent = ((double)m_current / m_total) * 100;

	int bars = percent / m_perBarPercent;

	if (bars == m_prevBars){
		return;
	}

	//remove old bars

	for (size_t i=0; i < m_prevNumPercentLen; ++i ) {
		m_stream << '\b';
	}

	for (int i=0;i < m_prevBars;++i){
		m_stream << '\b';
	}
	m_prevBars = bars;

	for (int i=0;i < bars;++i){
		m_stream << m_barMark;
	}

	size_t percentI = percent;

	m_stream << std::to_string(percentI) << "%";

	m_prevNumPercentLen = std::to_string(percentI).length()+1; // +1 for the percent sign

	m_stream.flush();

}


void ProgressBar::startOutput()
{
	if (m_isOutput){
		WARNSTDOUT("Output already started");
		return;
	}

	m_isOutput = true;
	m_stream << "\t >>>> " << m_caption << " <<<< \n";
	redraw();
}


void ProgressBar::stopOutput()
{
	if (m_isOutput){
		m_current = m_total;
		redraw();
		m_isOutput = false;
		m_stream << "\n";
	}
	else {
		WARNSTDOUT("Output already stopped");
	}
}
