#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <algorithm>
#include <assert.h>
#include <stack>

// TODO: make map from std::vector<RegularExpr>

enum SymbolType
{
	NONE,
	NT,
	T,
	SPECIAL,
};

struct Symbol
{
	SymbolType type;
	std::string kind;
};

struct RegularExpr
{
	Symbol left;
	std::vector<std::vector<Symbol>> right;

	bool hasEpsilon;
};

std::vector<RegularExpr> regularExpressions;
Symbol startNT;

struct FirstSet
{
	Symbol left;
	std::vector<std::vector<Symbol>> right;

	bool hasEpsilon;
};

void insertSymbol(const Symbol &s, std::vector<Symbol> &result)
{
	if (std::find_if(result.begin(), result.end(), // NOTE: pretty much only for epsilon copies
		[&s](const Symbol &a) {return a.kind == s.kind; }) == result.end())
	{
		result.push_back(s);
	}
}

bool findSymbol(const Symbol &s, const std::vector<Symbol> symbols)
{
	if (std::find_if(symbols.begin(), symbols.end(), [&s](const Symbol &a) {return a.kind == s.kind; }) != symbols.end())
	{
		return true;
	}
	return false;
}

void firstA(const std::vector<Symbol> &block, std::vector<Symbol> &result);

void firstB(const Symbol &s, std::vector<Symbol> &result)
{
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		if (regularExpressions[i].left.kind == s.kind)
		{
			for (int j = 0; j < regularExpressions[i].right.size(); j++)
			{
				firstA(regularExpressions[i].right[j], result);
			}
		}
	}
}

void firstA(const std::vector<Symbol> &block, std::vector<Symbol> &result)
{
	for (int i = 0; i < block.size(); i++)
	{
		if (block[i].type == T)
		{
			insertSymbol(block[i], result);
			return;
		}
		else
		{
			bool hasEpsilon = false;
			for (int j = 0; j < regularExpressions.size(); j++)
			{
				if (regularExpressions[j].left.kind == block[i].kind)
				{
					hasEpsilon = regularExpressions[j].hasEpsilon;
					break;
				}
			}

			firstB(block[i], result);

			if (!hasEpsilon)
			{
				return;
			}
		}
	}
}

void first(const Symbol &s, std::vector<std::vector<Symbol>> &result)
{
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		if (regularExpressions[i].left.kind == s.kind)
		{
			for (int j = 0; j < regularExpressions[i].right.size(); j++)
			{
				std::vector<Symbol> block = {};
				firstA(regularExpressions[i].right[j], block);
				result.push_back(block);
			}
		}
	}
}

struct FollowSet
{
	Symbol left;
	std::vector<Symbol> right;
};

void follow(const Symbol &s, std::vector<Symbol> &result);

void followE(const Symbol &left, const std::vector<Symbol> &right, int pos, std::vector<Symbol> &result)
{
	if (pos + 1 < right.size())
	{
		if (right[pos + 1].type == T)
		{
			insertSymbol(right[pos + 1], result);
		}
		else
		{
			std::vector<std::vector<Symbol>> firstSet = {};
			first(right[pos + 1], firstSet); // TODO: Use already calculated first sets for speed

			bool hasEpsilon = false;
			for (int i = 0; i < firstSet.size(); i++)
			{
				if (std::find_if(firstSet[i].begin(), firstSet[i].end(),
					[](const Symbol &a) {return a.kind == "e"; }) != firstSet[i].end())
				{
					hasEpsilon = true;
					break;
				}
			}

			for (int i = 0; i < firstSet.size(); i++)
			{
				for (int j = 0; j < firstSet[i].size(); j++)
				{
					if (firstSet[i][j].kind != "e" && !findSymbol(firstSet[i][j], result))
					{
						insertSymbol(firstSet[i][j], result);
					}
				}

			}

			if (hasEpsilon)
			{
				followE(left, right, pos + 1, result);
			}
		}
	}
	else
	{
		follow(left, result);
	}
}

void follow(const Symbol &s, std::vector<Symbol> &result)
{
	if (s.kind == startNT.kind)
	{
		insertSymbol(Symbol{ T, "$" }, result);
	}

	for (int i = 0; i < regularExpressions.size(); i++)
	{
		for (int j = 0; j < regularExpressions[i].right.size(); j++)
		{
			for (int k = 0; k < regularExpressions[i].right[j].size(); k++)
			{
				if (regularExpressions[i].right[j][k].kind == s.kind
					&& regularExpressions[i].left.kind != s.kind)
				{
					followE(regularExpressions[i].left, regularExpressions[i].right[j], k, result);
				}
			}
		}
	}
}

SymbolType stringToSymType(const std::string &type)
{
	if (type == "T")
	{
		return T;
	}
	else if (type == "SPECIAL")
	{
		return SPECIAL;
	}
	else if (type == "NT")
	{
		return NT;
	}
	else
	{
		return NONE;
	}
}


std::string next(const char **p)
{
begin:
	switch (**p)
	{
	case ' ':
	{
		(*p)++;
		goto begin;
	}
	break;
	default:
	{
		const char *begin = *p;
		while (**p && **p != ' ')
		{
			(*p)++;
		}
		const char *end = *p;

		return std::string(begin, end - begin);
	}
	break;
	}
}

RegularExpr parseRE(const std::string &line)
{
	const char *p = line.c_str();

	std::string nt = next(&p);
	assert("->" == next(&p));

	std::vector<Symbol> block = {};
	std::vector<std::vector<Symbol>> right = {};
	bool hasEpsilon = false;
	while (*p != '\0')
	{
		std::string type = next(&p);
		std::string kind = next(&p);

		if (kind == "e")
		{
			hasEpsilon = true;
		}
		else if (kind == "|")
		{
			right.push_back(block);
			block.clear();
		}

		if (kind != "|")
		{
			block.emplace_back(Symbol{ stringToSymType(type), kind });
		}
	}
	right.push_back(block);

	return RegularExpr{ Symbol{ NT, nt }, right, hasEpsilon };
}

void dumpRegularExpressions()
{
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		std::cout << regularExpressions[i].left.kind << " -> ";
		for (int j = 0; j < regularExpressions[i].right.size(); j++)
		{
			for (int k = 0; k < regularExpressions[i].right[j].size(); k++)
			{
				std::cout << regularExpressions[i].right[j][k].kind << ' ';
			}
			if (j != regularExpressions[i].right.size() - 1)
			{
				std::cout << "| ";
			}
		}
		std::cout << '\n';
	}
	std::cout << '\n';
}

void initRegularExpr()
{
	std::cout << "Enter regular expressions: (type 'exit' to stop)\n";

	std::string input = {};
	while (std::getline(std::cin, input) && input != "exit")
	{
		RegularExpr re = parseRE(input);
		regularExpressions.push_back(re);
	};

	dumpRegularExpressions();

	std::cout << "Enter start nonterminal: ( ";
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		std::cout << regularExpressions[i].left.kind << ' ';
	}
	std::cout << ")\n";

	std::cin >> input;
	startNT = { NT, input };
}

int findRegularExpr(const Symbol &s)
{
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		if (regularExpressions[i].left.kind == s.kind)
		{
			return i;
		}
	}
	return -1;
}

std::stack<Symbol> stack;

int parseInput(const std::string &s, std::map<std::string, std::map<std::string, std::vector<Symbol>>> parsingTable)
{
	const char *p = s.c_str();

	while (!stack.empty())
	{
		std::string sym = next(&p);
		p -= sym.length();

		if (stack.top().kind == sym)
		{
			std::cout << "Matched symbols: " << sym << '\n';
			p += sym.length();
			stack.pop();
		}
		else
		{
			std::vector<Symbol> production = parsingTable[stack.top().kind][sym];
			if (production.size() != 0)
			{
				stack.pop();

				for (int i = production.size() - 1; i >= 0; i--)
				{
					stack.push(production[i]);
				}
			}
			else
			{
				std::cout << "fatal: undefined parsing table entry\n";
				return 1;
			}
		}
	}

	return 0;
}

int main()
{
	// Init regular expressions
	initRegularExpr();

	// First sets
	std::vector<FirstSet> firstSets = {};
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		std::vector<std::vector<Symbol>> firstSet = {};
		first(regularExpressions[i].left, firstSet);
		firstSets.emplace_back(FirstSet{ regularExpressions[i].left, firstSet });
	}

	std::cout << "First: \n";
	for (auto i : firstSets)
	{
		std::cout << i.left.kind << ": {";
		for (auto j : i.right)
		{
			std::cout << " { ";
			for (auto k : j)
			{
				std::cout << k.kind << ' ';
			}
			std::cout << "}";
		}
		std::cout << " }\n";
	}

	// Follow sets
	std::vector<FollowSet> followSets = {};
	for (int i = 0; i < regularExpressions.size(); i++)
	{
		std::vector<Symbol> followSet = {};
		follow(regularExpressions[i].left, followSet);
		followSets.emplace_back(FollowSet{ regularExpressions[i].left, followSet });
	}

	std::cout << "Follow: \n";
	for (auto i : followSets)
	{
		std::cout << i.left.kind << ": { ";
		for (auto j : i.right)
		{
			std::cout << j.kind << ' ';
		}
		std::cout << "}\n";
	}

	// Creating parsing table 
	// TODO: add second rule
	std::map<std::string, std::map<std::string, std::vector<Symbol>>> parsingTable = {};
	for (int i = 0; i < firstSets.size(); i++)
	{
		for (int j = 0; j < firstSets[i].right.size(); j++)
		{
			for (int k = 0; k < firstSets[i].right[j].size(); k++)
			{
				int index = findRegularExpr(firstSets[i].left);
				assert(index != -1);

				std::string nt = firstSets[i].left.kind;
				std::string t = firstSets[i].right[j][k].kind;

				parsingTable[nt][t] = regularExpressions[index].right[j];
			}
		}
	}

	std::cout << "Parsing table: \n";
	for (auto i : parsingTable)
	{
		std::cout << i.first << ": { ";
		for (auto j : i.second)
		{
			std::cout << j.first << ' ';
		}
		std::cout << "} -> ";
		for (auto j : i.second)
		{
			std::cout << "{ ";
			for (auto k : j.second)
			{
				std::cout << k.kind << ' ';
			}
			std::cout << "} ";
		}
		std::cout << "}\n";
	}

	// Parsing
	std::cout << "Parsing:\nEnter strings to parse: (type 'exit' to stop)\n";

	std::string src = {};
	while (std::getline(std::cin, src) && src != "exit")
	{
		if (src != "")
		{
			// Stack initialization
			stack.push(Symbol{ T, "\0" });
			stack.push(startNT);

			if (parseInput(src, parsingTable) == 0)
			{
				std::cout << "Parsed successfully\n";
			}
			else
			{
				// Clean the stack
				stack = std::stack<Symbol>();
			}
		}
	}
}
