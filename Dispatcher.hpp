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

	virtual ~Dispatcher()
	{
	}
};

typedef Dispatcher<ValChangedVisitor> ValUpdater;
