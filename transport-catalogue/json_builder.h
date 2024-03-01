#pragma once

#include "json.h"

#include <optional>
#include <vector>

namespace json {

class Builder {
private:
	class BaseContext;
	class DictItemContext;
	class DictValueContext;
	class ArrayItemContext;
public:
	BaseContext Value(Node::Value value);
	DictItemContext StartDict();
	ArrayItemContext StartArray();
	DictValueContext Key(std::string key);

	BaseContext EndDict();
	BaseContext EndArray();

	Node Build();
private:
	Node root_;
	std::vector<Node*> nodes_stack_;
	Node GetNode(Node::Value value);

	std::optional<std::string> key_;

	class BaseContext {
	public:
		BaseContext(Builder& builder);

		Node Build();
		DictValueContext Key(std::string key);
		BaseContext Value(Node::Value value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		BaseContext EndDict();
		BaseContext EndArray();
	private:
		Builder& builder_;
	};

	class DictValueContext : public BaseContext {
	public:
		DictValueContext(BaseContext base)
			:BaseContext(base)
		{
		}

		Node Build() = delete;
		DictValueContext Key(std::string key) = delete;
		BaseContext EndDict() = delete;
		BaseContext EndArray() = delete;

		DictItemContext Value(Node::Value value);
	};

	class DictItemContext : public BaseContext {
	public:
		DictItemContext(BaseContext base)
			:BaseContext(base)
		{
		}
		Node Build() = delete;
		BaseContext Value(Node::Value value) = delete;
		DictItemContext StartDict() = delete;
		ArrayItemContext StartArray() = delete;
		BaseContext EndArray() = delete;
	};

	class ArrayItemContext : public BaseContext {
	public:
		ArrayItemContext(BaseContext base)
			:BaseContext(base)
		{
		}

		Node Build() = delete;
		DictValueContext Key(std::string key) = delete;
		BaseContext EndDict() = delete;

		ArrayItemContext Value(Node::Value value);
	};
};

} // namespace json