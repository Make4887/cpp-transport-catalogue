#pragma once

#include "json.h"

#include <optional>
#include <vector>

namespace json {

namespace detail {

class DictItemContext;
class KeyItemContext;
class ArrayItemContext;

} // detail

class Builder {
public:
	Builder& Value(Node::Value value);
	detail::DictItemContext StartDict();
	detail::ArrayItemContext StartArray();
	detail::KeyItemContext Key(std::string key);

	Builder& EndDict();
	Builder& EndArray();

	Node Build();
private:
	Node root_;
	std::vector<Node*> nodes_stack_;
	Node GetNode(Node::Value value);

	std::optional<std::string> key_;
};

namespace detail {

class ItemContext {
public:
	ItemContext(Builder& builder)
		:builder_(builder)
	{
	}
protected:
	Builder& builder_;
};

class DictValueItemContext : public ItemContext {
public:
	DictValueItemContext(Builder& builder)
		:ItemContext(builder)
	{
	}
	KeyItemContext Key(std::string key);
	Builder& EndDict();
};

class KeyItemContext : public ItemContext {
public:
	KeyItemContext(Builder& builder)
		:ItemContext(builder)
	{
	}
	DictValueItemContext Value(Node::Value value);
	DictItemContext StartDict();
	ArrayItemContext StartArray();
};

class DictItemContext : public ItemContext {
public:
	DictItemContext(Builder& builder)
		:ItemContext(builder)
	{
	}
	KeyItemContext Key(std::string key);
	Builder& EndDict();
};

class ArrayItemContext : public ItemContext {
public:
	ArrayItemContext(Builder& builder)
		:ItemContext(builder)
	{
	}
	ArrayItemContext Value(Node::Value value);
	DictItemContext StartDict();
	ArrayItemContext StartArray();
	Builder& EndArray();
};

} // detail

} // namespace json