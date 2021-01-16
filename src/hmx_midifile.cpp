#include "hmx_midifile.h"
#include <sstream>
#include <iomanip>

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


hmx_fusion_node::~hmx_fusion_node() {
	//if (auto node = std::get_if<hmx_fusion_nodes*>(&value)) {
	//	delete *node;
	//}
}

hmx_fusion_nodes hmx_fusion_parser::parseData(const std::vector<u8> &fusion_file) {
	const u8 *b = fusion_file.data();
	hmx_fusion_nodes nodes;

	auto consume = [&](char c) {
		if (*b == c) {
			++b;
		}
		else {
			__debugbreak();
		}
	};

	auto skip_whitespace = [&]() {
		while (isspace(*b)) { ++b; }
	};

	auto get_name = [&]() {
		std::string name;
		while (!isspace(*b)) {
			name += *b;
			++b;
		}

		return name;
	};

	auto get_string = [&]() {
		std::string str;
		
		consume('"');
		while (*b != '"') {
			str += *b;
			++b;
		}
		consume('"');

		return str;
	};

	auto get_number = [&]() {
		decltype(hmx_fusion_node::value) value;

		std::string number;
		bool has_dot = false;
		while ((*b >= '0' && *b <= '9') || *b == '.' || *b == '-') {
			number += *b;
			if (*b == '.') {
				has_dot = true;
			}
			++b;
		}

		if (has_dot) {
			value = std::stof(number);
		}
		else {
			value = std::stoi(number);
		}

		return value;
	};

	std::function<hmx_fusion_node()> parse_node;
	parse_node = [&]() {
		skip_whitespace();
		consume('(');
		hmx_fusion_node node;
		node.key = get_name();
		skip_whitespace();

		//Sub-Object
		if (*b == '(') {
			hmx_fusion_nodes* nodes = new hmx_fusion_nodes();
			while (*b == '(') {
				nodes->children.emplace_back(parse_node());
				skip_whitespace();
			}

			node.value = std::move(nodes);
		}
		//String
		else if (*b == '"') {
			node.value = get_string();
		}
		//Number or vector
		else {
			auto num = get_number();
			skip_whitespace();
			if (*b != ')') {
				auto num2 = get_number();

				hmx_vec v;
				v.x = std::get<float>(num);
				v.y = std::get<float>(num2);
				node.value = v;
			}
			else {
				node.value = num;
			}
		}

		skip_whitespace();
		consume(')');
		return node;
	};

	while ((b - fusion_file.data()) < fusion_file.size()) {
		nodes.children.emplace_back(parse_node());
		skip_whitespace();
	}

	return nodes;
}

std::string hmx_fusion_parser::outputData(const hmx_fusion_nodes &nodes) {
	std::string data;

	int indent = 0;
	bool apply_indent = false;
	auto insert_newline = [&](){
		data += "\n";
		apply_indent = true;
	};

	auto append = [&](const std::string &str) {
		if (apply_indent) {
			apply_indent = false;
			for (size_t i = 0; i < indent; ++i) {
				data += "   ";
			}
		}

		data += str;
	};

	std::function<void(const hmx_fusion_node &node)> outputNode;
	outputNode = [&](const hmx_fusion_node &node) {
		append("(");
		append(node.key);

		if (auto v_nodes = std::get_if<hmx_fusion_nodes*>(&node.value)) {
			++indent;
			insert_newline();
			for (auto &&n : (*v_nodes)->children) {
				outputNode(n);
			}
			--indent;
		}
		else if (auto str = std::get_if<std::string>(&node.value)) {
			++indent;
			insert_newline();
			append("\"");
			append(*str);
			append("\"");
			--indent;
			insert_newline();
		}
		else if (auto iValue = std::get_if<int>(&node.value)) {
			data += " " + std::to_string(*iValue);
		}
		else if (auto fValue = std::get_if<float>(&node.value)) {
			std::stringstream stream;
			stream << std::fixed << std::setprecision(4) << *fValue;
			std::string s = stream.str();
			
			append(" " + s);
		}
		else if (auto vecValue = std::get_if<hmx_vec>(&node.value)) {
			std::stringstream stream;
			stream << std::fixed << std::setprecision(4) << vecValue->x << " " << vecValue->y;
			std::string s = stream.str();

			append(" " + s);
		}

		append(")");
		insert_newline();
	};

	for (auto &&n : nodes.children) {
		outputNode(n);
	}

	return data;
}