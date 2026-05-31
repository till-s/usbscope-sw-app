/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/


#include <CalDacCtrl.hpp>

std::vector<QString> 
DACRangeTgl::labels( ScopeInterface *scp, int channel )
{
	std::vector<QString> rv;
	rv.push_back( (*scp->getChannelName(channel) + " hi-range") );
	rv.push_back( (*scp->getChannelName(channel) + " lo-range") );
	return rv;
}

DACRangeTgl::DACRangeTgl( ScopeInterface *scp, int channel, QWidget *parent )
: ScopeTglButton( scp, labels( scp, channel ), channel, parent )
{
	if ( dev()->currentParams()->afeParams[this->channel()].dacRangeHi < 0 ) {
		throw std::runtime_error("No DAC range controls");
	}
	updateGUI();
}

bool 
DACRangeTgl::getVal()
{
	return dev()->currentParams()->afeParams[channel()].dacRangeHi;
}
