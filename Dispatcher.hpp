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

#pragma once

// Double-dispatch for value changes

#include <list>

class TrigSrcMenu;
class TrigEdgMenu;
class TrigAutMenu;
class TrigArmMenu;
class TrigLevel;
class AttenuatorSlider;
class FECTerminationTgl;
class FECAttenuatorTgl;
class FECACCouplingTgl;
class ExtTrigOutEnTgl;
class ScaleXfrm;
class MeasMarker;
class NPreTriggerSamples;
class Decimation;
class Measurement;
class MeasDiff;
class CalDAC;
class DACRangeTgl;
class ClockGen;
class VersaClkFODRouter;
class VersaClkOutDiv;

class ValChangedVisitor
{
public:
	virtual void visit(TrigSrcMenu        *) {}
	virtual void visit(TrigEdgMenu        *) {}
	virtual void visit(TrigAutMenu        *) {}
	virtual void visit(TrigArmMenu        *) {}
	virtual void visit(TrigLevel          *) {}
	virtual void visit(AttenuatorSlider   *) {}
	virtual void visit(FECTerminationTgl  *) {}
	virtual void visit(FECAttenuatorTgl   *) {}
	virtual void visit(FECACCouplingTgl   *) {}
	virtual void visit(ExtTrigOutEnTgl    *) {}
	virtual void visit(ScaleXfrm          *) {}
	virtual void visit(MeasMarker         *) {}
	virtual void visit(NPreTriggerSamples *) {}
	virtual void visit(Decimation         *) {}
	virtual void visit(Measurement        *) {}
	virtual void visit(MeasDiff           *) {}
	virtual void visit(CalDAC             *) {}
	virtual void visit(DACRangeTgl        *) {}
	virtual void visit(ClockGen           *) {}
	virtual void visit(VersaClkOutDiv     *) {}
	virtual void visit(VersaClkFODRouter  *) {}

	virtual ~ValChangedVisitor() = default;
};

template <typename T>
class Dispatcher {
	std::list<T*> subscribers_;
public:
	virtual void accept(T *v) = 0;

	virtual void valChanged()
	{
		for ( auto it = subscribers_.begin(); it != subscribers_.end(); ++it ) {
			accept( *it );
		}
	}

	virtual void subscribe(T *v)
	{
		subscribers_.push_back( v );
	}

	virtual void unsubscribe(T *v)
	{
		subscribers_.remove( v );
	}

	virtual ~Dispatcher() = default;
};

using ValUpdater = Dispatcher<ValChangedVisitor>;

class ParamValUpdater;

// Don't use the Dispatcher template even though the functionality
// is very similar; the confusion of identical names becomes too great.
class ParamChangedVisitor : public virtual ValChangedVisitor {
	std::list<ParamValUpdater*> subscribers_;
public:
	virtual void updateGUI();

	// reverse subscription; every GUI element that dispatches to the ValUpdateVisitor
	// may be subscribed to the ValUpdateVisitor and triggered to update itself after
	// the visitor's state has changed.

	virtual void reverseSubscribe(ParamValUpdater *u)
	{
		subscribers_.push_back( u );
	}

	virtual void reverseUnsubscribe(ParamValUpdater *u)
	{
		subscribers_.remove( u );
	}

};

class ParamValUpdater : public virtual ValUpdater {
public:
	// propagate changes in hard/software up to the GUI
	virtual void updateGUI() = 0;

	// allow simple ValChangedVisitors to subscribe as well
	using ValUpdater::subscribe;
	using ValUpdater::unsubscribe;

	virtual void subscribe(ParamChangedVisitor *v)
	{
		ValUpdater::subscribe( v );
		v->reverseSubscribe( this );
	}

	virtual void unsubscribe(ParamChangedVisitor *v)
	{
		v->reverseUnsubscribe( this );
		Dispatcher<ValChangedVisitor>::unsubscribe( v );
	}
};
