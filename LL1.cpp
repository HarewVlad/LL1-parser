#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <algorithm>
#include <assert.h>
#include <stack>

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
	if (std::find_if(result.begin(), result.end(),
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
			result.push_back(block[i]);
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
			first(right[pos + 1], firstSet);

			bool hasEpsilon = false;
			for (int i = 0; i < firstSet.size(); i++)
			{
				if (std::find_if(firstSet[i].begin(), firstSet[i].end(),
					[](const Symbol &a) {return a.kind == "epsilon"; }) != firstSet[i].end())
				{
					hasEpsilon = true;
					break;
				}
			}

			for (int i = 0; i < firstSet.size(); i++)
			{
				for (int j = 0; j < firstSet[i].size(); j++)
				{
					if (firstSet[i][j].kind != "epsilon" && !findSymbol(firstSet[i][j], result))
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
		insertSymbol(Symbol{ T, "" }, result);
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

RegularExpr parseRE(const std::string &line)
{
	std::stringstream ss {line};
	std::string nt, type, kind = {};
	std::string del = {};

	ss >> nt;
	ss >> del;
	assert(del == "->");

	std::vector<Symbol> block = {};
	std::vector<std::vector<Symbol>> right = {};
	bool hasEpsilon = false;
	while (!ss.eof())
	{
		ss >> type;
		assert(type == "NT" || type == "T" || type == "SPECIAL");
		ss >> kind;

		if (kind == "epsilon")
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

std::string next(const char **p, size_t size)
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
		while (**p && **p != ' ' && size--)
		{
			(*p)++;
		}
		const char *end = *p;

		return std::string(begin, end - begin);
	}
	break;
	}
}

int parseInput(const std::string &s, std::map<std::string, std::map<std::string, std::vector<Symbol>>> parsingTable)
{
	const char *p = s.c_str();

	size_t len = 1;
	while (!stack.empty())
	{
		std::string sym = next(&p, len);
		p -= len;

		if (stack.top().kind == sym)
		{
			std::cout << "Matched symbols: " << sym << '\n';
			p += len;
			stack.pop();

			len = 1;
		}
		else
		{
			std::vector<Symbol> production = parsingTable[stack.top().kind][sym];
			if (production.size() != 0)
			{
				stack.pop();

				for (int i = production.size() - 1; i >= 0; i--)
				{
					if (production[i].kind != "epsilon")
					{
						stack.push(production[i]);
					}
				}
			}
			else
			{
				len++;
				assert(len <= s.length());
			}
		}
	}

	return 0;
}

int findFollowSet(const std::vector<FollowSet> &followSets, const Symbol &s)
{
	for (int i = 0; i < followSets.size(); i++)
	{
		if (followSets[i].left.kind == s.kind)
		{
			return i;
		}
	}
	return -1;
}

int main()
{
	// Init regular expressions
	{
		std::cout << "Enter regular expressions: (type 'exit' to stop)\n";

		std::string input = {};
		while (std::getline(std::cin, input) && input != "exit")
		{
			RegularExpr re = parseRE(input);
			regularExpressions.push_back(re);
		};

		// Print
		std::cout << "--Regular expressions:\n";
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

		std::cout << "Enter start nonterminal: ( ";
		for (int i = 0; i < regularExpressions.size(); i++)
		{
			std::cout << regularExpressions[i].left.kind << ' ';
		}
		std::cout << ")\n";

		std::cin >> input;
		startNT = { NT, input };
	}

	// First sets
	std::vector<FirstSet> firstSets = {};
	{
		for (int i = 0; i < regularExpressions.size(); i++)
		{
			std::vector<std::vector<Symbol>> firstSet = {};
			first(regularExpressions[i].left, firstSet);
			firstSets.emplace_back(FirstSet{ regularExpressions[i].left, firstSet });
		}

		// Print
		std::cout << "--First: \n";
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
	}

	// Follow sets
	std::vector<FollowSet> followSets = {};
	{
		for (int i = 0; i < regularExpressions.size(); i++)
		{
			std::vector<Symbol> followSet = {};
			follow(regularExpressions[i].left, followSet);
			followSets.emplace_back(FollowSet{ regularExpressions[i].left, followSet });
		}

		// Print
		std::cout << "--Follow: \n";
		for (auto i : followSets)
		{
			std::cout << i.left.kind << ": { ";
			for (auto j : i.right)
			{
				if (j.kind == "")
				{
					std::cout << "$" << ' ';
				}
				else
				{
					std::cout << j.kind << ' ';
				}
			}
			std::cout << "}\n";
		}
	}

	// Creating parsing table 
	std::map<std::string, std::map<std::string, std::vector<Symbol>>> parsingTable = {};
	{
		for (int numSet = 0; numSet < firstSets.size(); numSet++)
		{
			int regularExprIndex = findRegularExpr(firstSets[numSet].left);
			assert(regularExprIndex != -1);

			for (int numRight = 0; numRight < firstSets[numSet].right.size(); numRight++)
			{
				for (int numSymFirst = 0; numSymFirst < firstSets[numSet].right[numRight].size(); numSymFirst++)
				{
					if (firstSets[numSet].right[numRight][numSymFirst].kind != "epsilon")
					{
						std::string nt = firstSets[numSet].left.kind;
						std::string t = firstSets[numSet].right[numRight][numSymFirst].kind;

						parsingTable[nt][t] = regularExpressions[regularExprIndex].right[numRight];
					}
					else
					{
						int followSetIndex = findFollowSet(followSets, firstSets[numSet].left);
						assert(followSetIndex != -1);

						for (int numSymFollow = 0; numSymFollow < followSets[followSetIndex].right.size(); numSymFollow++)
						{
							std::string nt = followSets[followSetIndex].left.kind;
							std::string t = followSets[followSetIndex].right[numSymFollow].kind;

							parsingTable[nt][t] = regularExpressions[regularExprIndex].right[numRight];
						}
					}
				}
			}
		}

		// Print
		std::cout << "--Parsing table: \n";
		for (auto i : parsingTable)
		{
			std::cout << i.first << ": { ";
			for (auto j : i.second)
			{
				if (j.first == "")
				{
					std::cout << "$" << ' ';
				}
				else
				{
					std::cout << j.first << ' ';
				}
			}
			std::cout << "} -> ";
			for (auto j : i.second)
			{
				std::cout << "{ ";
				for (auto k : j.second)
				{
					if (k.kind == "")
					{
						std::cout << "$" << ' ';
					}
					else
					{
						std::cout << k.kind << ' ';
					}
				}
				std::cout << "} ";
			}
			std::cout << "}\n";
		}
	}
	
	// Parsing
	{
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
	
}