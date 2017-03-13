#include <string.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

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

  explicit Symbol(const string& v) {
    if (v[0] == '\'') {
      type_ = Type::term;
      v_ = v.substr(1, v.size() - 2);
    } else {
      type_ = Type::no_term;
      v_ = v;
    }
  }

  Type type() const { return type_; }
  const string& v() const { return v_; }

 private:
  Type type_;
  string v_;
};

class Rule {
 public:
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

};

int main(int argc, char *argv[]) {

  return 0;
}
