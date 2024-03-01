#include "json_builder.h"

#include <cassert>
#include <string>

namespace json {

Builder::BaseContext::BaseContext(Builder& builder)
	:builder_(builder)
{
}

Node Builder::BaseContext::Build() {
	return builder_.Build();
}
Builder::DictValueContext Builder::BaseContext::Key(std::string key) {
	return builder_.Key(std::move(key));
}
Builder::BaseContext Builder::BaseContext::Value(Node::Value value) {
	return builder_.Value(std::move(value));
}
Builder::DictItemContext Builder::BaseContext::StartDict() {
	return builder_.StartDict();
}
Builder::ArrayItemContext Builder::BaseContext::StartArray() {
	return builder_.StartArray();
}
Builder::BaseContext Builder::BaseContext::EndDict() {
	return builder_.EndDict();
}
Builder::BaseContext Builder::BaseContext::EndArray() {
	return builder_.EndArray();
}

Builder::DictItemContext Builder::DictValueContext::Value(Node::Value value) {
	return BaseContext::Value(std::move(value));;
}

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
	return BaseContext::Value(std::move(value));
}

Node Builder::GetNode(Node::Value value) {
	if (std::holds_alternative<int>(value)) {
		return std::get<int>(value);
	}
	if (std::holds_alternative<double>(value)) {
		return std::get<double>(value);
	}
	if (std::holds_alternative<Dict>(value)) {
		return std::move(std::get<Dict>(value));
	}
	if (std::holds_alternative<Array>(value)) {
		return std::move(std::get<Array>(value));
	}
	if (std::holds_alternative<bool>(value)) {
		return std::get<bool>(value);
	}
	if (std::holds_alternative<std::string>(value)) {
		return std::move(std::get<std::string>(value));
	}
	return nullptr;
}

Builder::BaseContext Builder::Value(Node::Value value) {
	using namespace std::literals::string_literals;
	if (nodes_stack_.empty() && root_.IsNull()) {
		root_ = GetNode(value);
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		Array& array = std::get<Array>(nodes_stack_.back()->GetEtitableValue());
		array.emplace_back(GetNode(value));
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && key_) {
		Dict& dict = std::get<Dict>(nodes_stack_.back()->GetEtitableValue());
		dict.emplace(*key_, GetNode(value));
		key_.reset();
	}
	else {
		throw std::logic_error("Value not in the correct place"s);
	}
	return BaseContext{ *this };
}

Builder::DictItemContext Builder::StartDict() {
	using namespace std::literals::string_literals;
	if (nodes_stack_.empty() && root_.IsNull()) {
		root_ = Dict{};
		nodes_stack_.push_back(&root_);
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		Array& array = std::get<Array>(nodes_stack_.back()->GetEtitableValue());
		array.emplace_back(Dict{});
		nodes_stack_.push_back(&array.back());
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && key_) {
		Dict& dict = std::get<Dict>(nodes_stack_.back()->GetEtitableValue());
		dict.emplace(*key_, Dict{});
		nodes_stack_.push_back(&dict.at(*key_));
		key_.reset();
	}
	else {
		throw std::logic_error("Dict not in the correct place"s);
	}
	return BaseContext{ *this };
}

Builder::ArrayItemContext Builder::StartArray() {
	using namespace std::literals::string_literals;
	if (nodes_stack_.empty() && root_.IsNull()) {
		root_ = Array{};
		nodes_stack_.push_back(&root_);
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		Array& array = std::get<Array>(nodes_stack_.back()->GetEtitableValue());
		array.emplace_back(Array{});
		nodes_stack_.push_back(&array.back());
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && key_) {
		Dict& dict = std::get<Dict>(nodes_stack_.back()->GetEtitableValue());
		dict.emplace(*key_, Array{});
		nodes_stack_.push_back(&dict.at(*key_));
		key_.reset();
	}
	else {
		throw std::logic_error("Array not in the correct place"s);
	}
	return BaseContext{ *this };
}

Builder::DictValueContext Builder::Key(std::string key) {
	using namespace std::literals::string_literals;
	if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && !key_) {
		key_ = key;
	}
	else {
		throw std::logic_error("Key not in the correct place"s);
	}
	return BaseContext{ *this };
}

Builder::BaseContext Builder::EndDict() {
	using namespace std::literals::string_literals;
	if (!nodes_stack_.empty() && nodes_stack_.back()->IsDict() && !key_) {
		nodes_stack_.pop_back();
	}
	else {
		throw std::logic_error("EndDict not in the correct place"s);
	}
	return BaseContext{ *this };
}

Builder::BaseContext Builder::EndArray() {
	using namespace std::literals::string_literals;
	if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		nodes_stack_.pop_back();
	}
	else {
		throw std::logic_error("EndDict not in the correct place"s);
	}
	return BaseContext{ *this };
}

Node Builder::Build() {
	using namespace std::literals::string_literals;
	if (nodes_stack_.empty() && !root_.IsNull()) {
		return root_;
	}
	else {
		throw std::logic_error("Build not in the correct place"s);
	}
}

} // namespace json