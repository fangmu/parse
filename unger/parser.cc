#include <string.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

using std::string;
using std::vector;
using std::map;

#define LOG std::cerr << __FUNCTION__ << ":" << __LINE__ << "\t"

namespace str {

void Split(char* str, char delimiter, bool ignore_empty,
           vector<char*>* vec) {
  if (str == NULL || vec == NULL) {
    return;
  }
  vec->clear();
  char* b = str;
  char* p = b;
  while (p != NULL && *p !='\0') {
    p = strchr(b, delimiter);
    if (p != NULL) {
      if (p != b || !ignore_empty) {
        (*p) = 0;
        vec->push_back(b);
      }
      p += 1;
      b = p;
    } else {
      vec->push_back(b);
    }
  }
  if (p == b && !ignore_empty) {
    vec->push_back(p);
  }
}

char* Trim(char* str) {
  if (*str == '\0') return str;

  char* tail = str + strlen(str) - 1;
  while ((tail > str) && (isspace(*tail) != 0)) --tail;
  *(tail+1) = '\0';

  while (*str != '\0' && (isspace(*str) != 0)) ++str;
  return str;
}

}  // namespace str

class Symbol {
 public:
  enum class Type {term, no_term};

  Symbol() = default;

  explicit Symbol(const string& v) {
    string s(str::Trim(const_cast<char*>(v.c_str())));

    if (s[0] == '\'') {
      type_ = Type::term;
      v_ = s.substr(1, s.size() - 2);
    } else {
      type_ = Type::no_term;
      v_ = s;
    }
  }

  Type type() const { return type_; }
  const string& v() const { return v_; }

  friend bool operator < (const Symbol& lhs, const Symbol& rhs) {
    return lhs.v_ < rhs.v_;
  }

  friend bool operator == (const Symbol& lhs, const Symbol& rhs) {
    return (lhs.type_ == rhs.type_) && (lhs.v_ == rhs.v_);
  }

 private:
  Type type_;
  string v_;
};

bool operator != (const Symbol& lhs, const Symbol& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator << (std::ostream& os, const Symbol& symbol) {
  if (symbol.type() == Symbol::Type::term) {
    os << "'" << symbol.v() << "'";
  } else {
    os << symbol.v();
  }
  return os;
}

class Rule {
 public:
  Rule() = default;
  Rule(const string& lhs, const string& rhs)
      : lhs_(lhs) {
    ParseRHS(rhs);
  }

  const Symbol& lhs() const { return lhs_; }
  const vector<vector<Symbol>>& derivations() const { return derivations_; }

 private:
  void ParseRHS(const string& rhs) {
    char* str = const_cast<char*>(rhs.c_str());
    vector<char*> sections;
    str::Split(str, '|', true, &sections);

    for (size_t i = 0; i < sections.size(); ++i) {
      vector<char*> symstrs;
      str::Split(sections[i], ' ', true, &symstrs);
      if (symstrs.size() == 0) continue;

      vector<Symbol> symbols;
      for (size_t j = 0; j < symstrs.size(); ++j) {
        symbols.emplace_back(Symbol(string(str::Trim(symstrs[j]))));
      }
      derivations_.emplace_back(symbols);
    }
  }

 private:
  Symbol lhs_;
  vector<vector<Symbol>> derivations_;
};

class Grammer {
 public:
  explicit Grammer(const string& file) {
    std::ifstream ifs(file.c_str());

    string line;
    bool first_line = true;
    while (getline(ifs, line)) {
      size_t l = line.find_first_of("->");
      string lhs = line.substr(0, l-1);
      string rhs = line.substr(l+2);

      Rule rule(lhs, rhs);
      rules_[rule.lhs()] = rule;

      if (first_line) {
        first_line = false;
        start_ = rule.lhs();
      }
    }
  }

  string DebugString() const {
    std::stringstream ss;
    ss << "start: " << start_.v() << "\n";
    for (const auto& rule : rules_) {
      ss << rule.first.v() << "->";
      for (const auto& r : rule.second.derivations()) {
        for (const auto& s : r) {
          ss << s.v() << " ";
        }
        ss.seekp(-1, ss.cur);
        ss << "|";
      }
      ss.seekp(-1, ss.cur);
      ss << "\n";
    }

    return ss.str();
  }

  const Symbol& start() const { return start_; }

  const Rule& FindRule(const Symbol& symbol) const {
    auto it = rules_.find(symbol);
    return it->second;
  }

 private:
  Symbol start_;
  map<Symbol, Rule> rules_;
};

class Goal {
 public:
  Goal(const Symbol& symbol, const string& input,
       int32_t start, int32_t len)
      : symbol_(symbol),
        input_(input),
        start_(start),
        len_(len) {}

  bool operator == (const Goal& o) {
    if (o.symbol_ != symbol_) return false;

    return o.input_.substr(o.start_, o.len_).compare(
        input_.substr(start_, len_)) == 0;
  }

  friend bool operator < (const Goal& lhs, const Goal& rhs) {
    if (lhs.symbol_ == rhs.symbol_) {
      return lhs.input_.substr(lhs.start_, lhs.len_).compare(
          rhs.input_.substr(rhs.start_, rhs.len_)) < 0;
    }
    return lhs.symbol_ < rhs.symbol_;
  }

  friend std::ostream& operator <<(std::ostream& os, const Goal& goal) {
    os << goal.symbol_ << "->" << goal.input_.substr(goal.start_, goal.len_);
    return os;
  }

 private:
  const Symbol& symbol_;
  const string& input_;
  int32_t start_;
  int32_t len_;
};


class Parser {
 public:
  explicit Parser(const string& file) : grammer_(file) {}

  bool Parse(const string& input) {
    map<Goal, bool> goals;
    return ParseWithSymbol(grammer_.start(), input, 0, input.size(), &goals);
  }

 private:
  bool ParseWithSymbol(const Symbol& symbol, const string& input,
                       int32_t start, int32_t len,
                       map<Goal, bool>* goals) {
    LOG << "Check derivation: " << symbol << " -> '"
        << input.substr(start, len) << "'?" << std::endl;

    Goal goal{symbol, input, start, len};
    auto goals_it = goals->find(goal);
    if (goals_it != goals->end()) {
      LOG << "Already check goal: " << goal <<
          (goals_it->second ? " ok." : " wrong.") << std::endl;
      return goals_it->second;
    }
    LOG << "Add goal: " << goal << std::endl;
    (*goals)[goal] =  false;

    if (symbol.type() == Symbol::Type::term) {
      if ((symbol.v() == input.substr(start, len))) {
        LOG << "Check derivation: " << symbol << " -> '"
            << input.substr(start, len) << "' ok." << std::endl;
        (*goals)[goal] = true;
        return true;
      } else {
        LOG << "Check derivation: " << symbol << " -> '"
            << input.substr(start, len) << "' wrong." << std::endl;
        return false;
      }
    }

    const Rule& rule = grammer_.FindRule(symbol);

    for (const auto& derivation : rule.derivations()) {
      if (ParseWithDerivation(derivation, 0, input, start, len, goals)) {
        LOG << "Check derivation: " << symbol << " -> '"
            << input.substr(start, len) << "' ok." << std::endl;
        (*goals)[goal] = true;
        return true;
      }
    }

    LOG << "Check derivation: " << symbol << " -> '"
        << input.substr(start, len) << "' wrong." << std::endl;

    return false;
  }

  bool ParseWithDerivation(const vector<Symbol>& derivation,
                           int32_t symbol_index, const string& input,
                           int32_t start, int32_t len,
                           map<Goal, bool>* goals) {
    string debug;
    for (size_t i = symbol_index; i < derivation.size(); ++i) {
      debug.append(derivation[i].v());
      debug.append(" ");
    }
    LOG << "Check derivation: " << debug << " -> '"
        << input.substr(start, len) << "'?" << std::endl;

    int32_t symbol_size = derivation.size() - symbol_index;
    if (symbol_size > len) {
      LOG << "Check derivation: " << debug << " -> '"
          << input.substr(start, len) << "' wrong." << std::endl;
      return false;

    } else if (symbol_size == len) {
      bool ret = true;
      for (int32_t i = 0; i < len; ++i) {
        ret = ret && ParseWithSymbol(derivation[symbol_index + i],
                                     input, start + i, 1, goals);
      }
      if (ret) {
        LOG << "Check derivation: " << debug << " -> '"
            << input.substr(start, len) << "' ok." << std::endl;
      } else {
        LOG << "Check derivation: " << debug << " -> '"
            << input.substr(start, len) << "' false." << std::endl;
      }
      return ret;
    } else if (symbol_size == 1) {
        bool ret = ParseWithSymbol(derivation[symbol_index], input,
                                   start, len, goals);
        if (ret) {
          LOG << "Check derivation: " << debug << " -> '"
              << input.substr(start, len) << "' ok." << std::endl;
        } else {
          LOG << "Check derivation: " << debug << " -> '"
              << input.substr(start, len) << "' false." << std::endl;
        }
        return ret;
    } else {
      for (int32_t i = 0; i <= len - symbol_size; ++i) {
        if (ParseWithSymbol(derivation[symbol_index], input,
                            start, i + 1, goals) &&
            ParseWithDerivation(derivation, symbol_index+1,
                                input, start + i + 1, len - i - 1,
                                goals)) {
          LOG << "Check derivation: " << debug << " -> '"
              << input.substr(start, len) << "' ok." << std::endl;
          return true;
        }
      }

      LOG << "Check derivation: " << debug << " -> '"
          << input.substr(start, len) << "' false." << std::endl;
      return false;
    }
  }

 private:
  Grammer grammer_;
};

int main(int argc, char *argv[]) {
  if (argc < 3) {
    LOG << argv[0] << " grammer_file  input_string"
        << std::endl;
    return -1;
  }

  Parser parser(argv[1]);

  if (parser.Parse(argv[2])) {
    std::cerr << "Check ok!" << std::endl;
  } else {
    std::cerr << "Check wrong!" << std::endl;
  }

  return 0;
}
