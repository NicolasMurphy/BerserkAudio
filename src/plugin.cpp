#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelScrambler);
	p->addModel(modelXenizer);
	p->addModel(modelCombverb);
#ifndef METAMODULE
	p->addModel(modelXenScribe);
#endif
}
