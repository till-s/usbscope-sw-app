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
