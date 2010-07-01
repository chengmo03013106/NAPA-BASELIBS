/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke
 *   robert.birke@polito.it
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 ***********************************************************************/

#include "counter_measure.h"

CounterMeasure::CounterMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
};

CounterMeasure::~CounterMeasure() {
}


result CounterMeasure::RxPkt(result *r, ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f", r[R_RECEIVE_TIME]);
	debugOutput(dbg);
	return 1;
}

result CounterMeasure::RxData(result *r, ExecutionList *el) {
	return RxPkt(r,el);
}

result CounterMeasure::TxPkt(result *r, ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f", r[R_SEND_TIME]);
	debugOutput(dbg);
	return 1;
}

result CounterMeasure::TxData(result *r, ExecutionList *el) {
	return TxPkt(r,el);
}

CounterMeasurePlugin::CounterMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Counter";
	desc = "Pkts/chunks transmitted/received";
	id = COUNTER;
	/* end of mandatory properties */
}
