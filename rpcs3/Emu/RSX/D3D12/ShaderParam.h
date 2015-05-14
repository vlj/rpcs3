#pragma once
#if defined(DX12_SUPPORT)
#include <string>
#include <vector>

enum ParamFlag
{
	PARAM_IN,
	PARAM_OUT,
	PARAM_UNIFORM,
	PARAM_CONST,
	PARAM_NONE,
	PARAM_COUNT,
};

struct ParamItem
{
	std::string name;
	std::string value;
	int location;

	ParamItem(const std::string& _name, int _location, const std::string& _value = "")
		: name(_name)
		, value(_value),
		location(_location)
	{ }
};

struct ParamType
{
	const ParamFlag flag;
	std::string type;
	std::vector<ParamItem> items;

	ParamType(const ParamFlag _flag, const std::string& _type)
		: flag(_flag)
		, type(_type)
	{
	}

	bool SearchName(const std::string& name)
	{
		for (u32 i = 0; i<items.size(); ++i)
		{
			if (items[i].name.compare(name) == 0) return true;
		}

		return false;
	}
};

struct ParamArray
{
	std::vector<ParamType> params[PARAM_COUNT];

	ParamType* SearchParam(const ParamFlag &flag, const std::string& type)
	{
		for (u32 i = 0; i<params[flag].size(); ++i)
		{
			if (params[flag][i].type.compare(type) == 0)
				return &params[flag][i];
		}

		return nullptr;
	}

	bool HasParam(const ParamFlag flag, std::string type, const std::string& name)
	{
		ParamType* t = SearchParam(flag, type);
		return t && t->SearchName(name);
	}

	std::string AddParam(const ParamFlag flag, std::string type, const std::string& name, const std::string& value)
	{
		ParamType* t = SearchParam(flag, type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, -1, value);
		}
		else
		{
			params[flag].emplace_back(flag, type);
			params[flag].back().items.emplace_back(name, -1, value);
		}

		return name;
	}

	std::string AddParam(const ParamFlag flag, std::string type, const std::string& name, int location = -1)
	{
		ParamType* t = SearchParam(flag, type);

		if (t)
		{
			if (!t->SearchName(name)) t->items.emplace_back(name, location);
		}
		else
		{
			params[flag].emplace_back(flag, type);
			params[flag].back().items.emplace_back(name, location);
		}

		return name;
	}
};

class ShaderVar
{
public:
	std::string name;
	std::vector<std::string> swizzles;

	ShaderVar() = default;
	ShaderVar(const std::string& var)
	{
		auto var_blocks = fmt::split(var, { "." });

		if (var_blocks.size() == 0)
		{
			assert(0);
		}

		name = var_blocks[0];

		if (var_blocks.size() == 1)
		{
			swizzles.push_back("xyzw");
		}
		else
		{
			swizzles = std::vector<std::string>(var_blocks.begin() + 1, var_blocks.end());
		}
	}

	size_t get_vector_size() const
	{
		return swizzles[swizzles.size() - 1].length();
	}

	ShaderVar& symplify()
	{
		std::unordered_map<char, char> swizzle;

		static std::unordered_map<int, char> pos_to_swizzle =
		{
			{ 0, 'x' },
			{ 1, 'y' },
			{ 2, 'z' },
			{ 3, 'w' }
		};

		for (auto &i : pos_to_swizzle)
		{
			swizzle[i.second] = swizzles[0].length() > i.first ? swizzles[0][i.first] : 0;
		}

		for (int i = 1; i < swizzles.size(); ++i)
		{
			std::unordered_map<char, char> new_swizzle;

			for (auto &sw : pos_to_swizzle)
			{
				new_swizzle[sw.second] = swizzle[swizzles[i].length() <= sw.first ? '\0' : swizzles[i][sw.first]];
			}

			swizzle = new_swizzle;
		}

		swizzles.clear();
		std::string new_swizzle;

		for (auto &i : pos_to_swizzle)
		{
			if (swizzle[i.second] != '\0')
				new_swizzle += swizzle[i.second];
		}

		swizzles.push_back(new_swizzle);

		return *this;
	}

	std::string get() const
	{
		if (swizzles.size() == 1 && swizzles[0] == "xyzw")
		{
			return name;
		}

		return name + "." + fmt::merge({ swizzles }, ".");
	}
};
#endif