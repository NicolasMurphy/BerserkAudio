#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelCombverb);
	p->addModel(modelScrambler);
	p->addModel(modelXenizer);
#ifndef METAMODULE
	p->addModel(modelXenScribe);
#endif
}
