#include "hmx_midifile.h"

void hmx_array::serialize(DataBuffer &buffer) {
	numChildren = children.size();

	buffer.serialize(nodeId);
	buffer.serialize(numChildren);
	buffer.serialize(unk);
	buffer.serializeWithSize(children, numChildren);
}

void hmx_subtree_node::serialize(DataBuffer &buffer) {
	numChildren = children.size();

	buffer.serialize(numChildren);
	buffer.serialize(nodeId);
	buffer.serializeWithSize(children, numChildren);
}