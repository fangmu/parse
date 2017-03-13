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


 private:
  Symbol start_;
  map<Symbol, Rule> rules_;
};

int main(int argc, char *argv[]) {
  Grammer gramer(argv[1]);

  std::cout << gramer.DebugString() << std::endl;
  return 0;
}
