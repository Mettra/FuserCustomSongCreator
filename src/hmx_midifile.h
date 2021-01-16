#pragma once

#include "core_types.h"
#include "serialize.h"

struct hmx_node;
struct hmx_fusion_nodes;

struct hmx_vec {
	float x;
	float y;
};

struct hmx_fusion_node {
	~hmx_fusion_node();
	std::string key;
	std::variant<int, float, std::string, hmx_fusion_nodes*, hmx_vec> value;
};

struct hmx_fusion_nodes {
	std::vector<hmx_fusion_node> children;


	hmx_fusion_node* getChild(const std::string &key) {
		for (auto &&c : children) {
			if (c.key == key) {
				return &c;
			}
		}

		return nullptr;
	}

	int& getInt(const std::string &key) {
		return std::get<int>(getChild(key)->value);
	}

	std::string& getString(const std::string &key) {
		return std::get<std::string>(getChild(key)->value);
	}

	hmx_fusion_nodes& getNode(const std::string &key) {
		return *std::get<hmx_fusion_nodes*>(getChild(key)->value);
	}

};

struct hmx_fusion_parser {
	static hmx_fusion_nodes parseData(const std::vector<u8> &fusion_file);
	static std::string outputData(const hmx_fusion_nodes &nodes);
};


struct hmx_array {
	i32 nodeId;
	u16 numChildren;
	u16 unk;
	std::vector<hmx_node> children;

	void serialize(DataBuffer &buffer);
};

struct hmx_subtree_node {
	u16 numChildren;
	i32 nodeId;
	std::vector<hmx_node> children;

	void serialize(DataBuffer &buffer);
};

struct hmx_string {
	std::string str;

	void serialize(DataBuffer &buffer) {
		i32 size = str.size();
		buffer.serialize(size);
		str.resize(size);
		buffer.serialize((u8*)str.data(), size);
	}
};

struct hmx_node {
	enum class Type : u32 {
		Int = 0x00,
		Float = 0x01,
		Name = 0x02,
		Keyword = 0x05,
		kDataUnhandled = 0x06,
		IfDef = 0x07,
		Else = 0x08,
		EndIf = 0x09,
		SubTree_Array = 0x10,
		SubTree_Nodes = 0x11,
		String = 0x12,
		SubTree_Unknown = 0x13,
		Define = 0x20,
		Include = 0x21,
		Merge = 0x22,
		IfNDef = 0x23
	};
	u32 type;

	std::variant<i32, float, hmx_string, hmx_array, hmx_subtree_node> value;

	hmx_array& getArray() {
		return std::get<hmx_array>(value);
	}

	hmx_string& getString() {
		return std::get<hmx_string>(value);
	}

	void serialize(DataBuffer &buffer) {
		buffer.serialize(type);

		if (buffer.loading) {
			switch (type) {
			case 0x00: // 32 bit int, 4 bytes
				value = i32();
				break;
			case 0x01: // 32 bit float, 4 bytes
				value = float();
				break;
			case 0x02: // variable name, 4 bytes specify size of string that follows
				value = hmx_string();
				break;
			case 0x05: // keyword (unquoted string), 4 bytes specify size of string that follows
				value = hmx_string();
				break;
			case 0x06: // kDataUnhandled marker, 4 empty bytes follow
				int null;
				break;
			case 0x07: // #ifdef directive, string, 4 bytes specify size of macro string that follows
				value = hmx_string();
				break;
			case 0x08: // #else directive, 4 empty bytes follow
				value = i32();
				break;
			case 0x09: // #endif directive, 4 empty bytes follow
				value = i32();
				break;
			case 0x10: // sub-tree, 6 bytes: 2 bytes = num children, 4 bytes = node id, bracketed with ()
				value = hmx_array();
				break;
			case 0x11: // sub-tree, 6 bytes: 2 bytes = num children, 4 bytes = node id, bracketed with {} - function calls/control structures
				value = hmx_subtree_node();
				break;
			case 0x12: // data string (quoted string), 4 bytes specify size of string that follows
				value = hmx_string();
				break;
			case 0x13: // sub-tree, 6 bytes: 2 bytes = num children, 4 bytes = node id, bracketed with [] - unknown purpose
				value = hmx_subtree_node();
				break;
			case 0x20: // #define directive, 4 bytes specify size of macro string that follows
				value = hmx_string();
				break;
			case 0x21: // #include directive, 4 bytes specify size of filename string that follows
				value = hmx_string();
				break;
			case 0x22: // #merge directive, 4 bytes specify size of filename string that follows
				value = hmx_string();
				break;
			case 0x23: // #ifndef directive, 4 bytes specify size of macro string that follows
				value = hmx_string();
				break;
			}
		}
		
		std::visit([&](auto &&v) {
			buffer.serialize(v);
		}, value);
	}
};