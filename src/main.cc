/*
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Licensed under the MIT License

Copyright (c) 2019 Brett Robinson <https://octobanana.com/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstdint>
#include <list>
#include <deque>
#include <string>
#include <vector>
#include <limits>
#include <utility>
#include <sstream>
#include <variant>
#include <iostream>
#include <iterator>
#include <functional>
#include <type_traits>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
using num = std::variant<i64, f64>;
using sym = std::string;
using atm = std::variant<num, sym>;
struct xpr;
using lst = std::list<xpr>;
using fun = std::function<xpr(xpr&)>;
struct xpr : std::variant<lst, fun, atm> {};
using env = std::unordered_map<sym, xpr>;
using Tokens = std::deque<std::string>;

#define xpr_atm(x) (std::get_if<atm>(&(x)))
#define xpr_num(x) (std::get_if<num>(std::get_if<atm>(&(x))))
#define xpr_i64(x) (std::get_if<i64>(std::get_if<num>(std::get_if<atm>(&(x)))))
#define xpr_f64(x) (std::get_if<f64>(std::get_if<num>(std::get_if<atm>(&(x)))))
#define xpr_sym(x) (std::get_if<sym>(std::get_if<atm>(&(x))))
#define xpr_lst(x) (std::get_if<lst>(&(x)))
#define xpr_fun(x) (std::get_if<fun>(&(x)))
#define atm_num(x) (std::get_if<num>(&(x)))
#define atm_sym(x) (std::get_if<sym>(&(x)))
#define atm_i64(x) (std::get_if<i64>(std::get_if<num>(&(x))))
#define atm_f64(x) (std::get_if<f64>(std::get_if<num>(&(x))))
#define num_i64(x) (std::get_if<i64>(&(x)))
#define num_f64(x) (std::get_if<f64>(&(x)))
#define i64_xpr(x) (xpr{atm{num{(x)}}})
#define f64_xpr(x) (xpr{atm{num{(x)}}})
#define sym_atm(x) (atm{sym{(x)}})
#define sym_xpr(x) (xpr{atm{sym{(x)}}})
#define holds(x, y) (std::holds_alternative<y>((x)))

std::string replace(std::string str, std::string const& key, std::string const& val, std::size_t size = std::numeric_limits<std::size_t>::max()) {
  for (std::size_t pos {0}; size-- > 0;) {
    pos = str.find(key, pos);
    if (pos == std::string::npos) {break;}
    str.replace(pos, key.size(), val);
    pos += val.size();
  }
  return str;
}

std::string replace(std::string str, std::vector<std::pair<std::string, std::string>> const& vals) {
  for (auto const& [key, val] : vals) {str = replace(str, key, val);}
  return str;
}

std::deque<std::string> split(std::string const& str, std::string const& delim, std::size_t size = std::numeric_limits<std::size_t>::max()) {
  std::string buf;
  std::deque<std::string> tok;
  std::size_t start {0};
  auto end = str.find(delim);
  while ((size-- > 0) && (end != std::string::npos)) {
    buf = str.substr(start, end - start);
    if (buf.size()) {tok.emplace_back(buf);}
    start = end + delim.size();
    end = str.find(delim, start);
  }
  buf = str.substr(start, end);
  if (buf.size()) {tok.emplace_back(buf);}
  return tok;
}

template<typename T, typename F1, typename F2> void for_each(T& t, F1 const& f1, F2 const& f2) {
  if (t.empty()) {return;}
  auto end = --t.end();
  for (auto i = t.begin(); i != end; ++i) {auto& e = *i; f1(e);}
  f2(t.back());
}

void env_init(env& ev) {
  auto const fc_if64 = [](xpr& x, auto fn) {
    auto& l = std::get<lst>(x);
    if (l.size() != 2) {throw std::runtime_error("unbalanced function arguments");}
    auto n1 = xpr_num(*l.begin());
    auto n2 = xpr_num(*std::next(l.begin(), 1));
    if (!n1 || !n2) {throw std::runtime_error("invalid type");}
    if (n1->index() == n2->index()) {
      if (holds(*n1, i64)) {return fn(*num_i64(*n1), *num_i64(*n2)) ? sym_xpr("#t") : sym_xpr("#f");}
      return fn(*num_f64(*n1), *num_f64(*n2)) ? sym_xpr("#t") : sym_xpr("#f");
    }
    if (holds(*n1, i64)) {return fn(static_cast<f64>(*num_i64(*n1)), *num_f64(*n2)) ? sym_xpr("#t") : sym_xpr("#f");}
    return fn(*num_f64(*n1), static_cast<f64>(*num_i64(*n2))) ? sym_xpr("#t") : sym_xpr("#f");
  };
  auto const fa_i64 = [](xpr& x, auto fn) {
    auto& l = std::get<lst>(x);
    if (l.size() != 2) {throw std::runtime_error("unbalanced function arguments");}
    auto n1 = xpr_i64(*l.begin());
    auto n2 = xpr_i64(*std::next(l.begin(), 1));
    if (!n1 || !n2) {throw std::runtime_error("invalid type");}
    return i64_xpr(fn(*n1, *n2));
  };
  auto const fa_if64 = [](xpr& x, auto fn) {
    auto& l = std::get<lst>(x);
    if (l.size() != 2) {throw std::runtime_error("unbalanced function arguments");}
    auto n1 = xpr_num(*l.begin());
    auto n2 = xpr_num(*std::next(l.begin(), 1));
    if (!n1 || !n2) {throw std::runtime_error("invalid type");}
    if (n1->index() == n2->index()) {
      if (holds(*n1, i64)) {return i64_xpr(fn(*num_i64(*n1), *num_i64(*n2)));}
      return f64_xpr(fn(*num_f64(*n1), *num_f64(*n2)));
    }
    if (holds(*n1, i64)) {return f64_xpr(fn(static_cast<f64>(*num_i64(*n1)), *num_f64(*n2)));}
    return f64_xpr(fn(*num_f64(*n1), static_cast<f64>(*num_i64(*n2))));
  };
#define op(X, Y) (X(x, [](auto const& n1, auto const& n2) {return n1 Y n2;}))
  ev["#t"] = sym_xpr("#t");
  ev["#f"] = sym_xpr("#f");
  ev["nil"] = sym_xpr("nil");
  ev["=="] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, ==);}};
  ev["!="] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, !=);}};
  ev["<"] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, <);}};
  ev["<="] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, <=);}};
  ev[">"] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, >);}};
  ev[">="] = xpr{[&](xpr& x) -> xpr {return op(fc_if64, >=);}};
  ev["*"] = xpr{[&](xpr& x) -> xpr {return op(fa_if64, *);}};
  ev["/"] = xpr{[&](xpr& x) -> xpr {return op(fa_if64, /);}};
  ev["-"] = xpr{[&](xpr& x) -> xpr {return op(fa_if64, -);}};
  ev["+"] = xpr{[&](xpr& x) -> xpr {return op(fa_if64, +);}};
  ev["%"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, %);}};
  ev["&"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, &);}};
  ev["|"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, |);}};
  ev["^"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, ^);}};
  ev["<<"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, <<);}};
  ev[">>"] = xpr{[&](xpr& x) -> xpr {return op(fa_i64, >>);}};
#undef op
}

Tokens str_tokens(std::string const& str) {
  return split(replace(str, {{"(", " ( "}, {")", " ) "}}), " ");
}

atm token_atom(std::string const& token) {
  try {
    if (token.find(".") != std::string::npos) {
      return atm{num{static_cast<f64>(std::stod(token))}};
    }
    return atm{num{static_cast<i64>(std::stol(token))}};
  }
  catch (...) {return sym_atm(token);}
}

xpr tokens_xpr(Tokens& tokens) {
  if (tokens.empty()) {throw std::runtime_error("unexpected EOF");}
  auto token = tokens.front(); tokens.pop_front();
  if (token == "(") {
    xpr x;
    auto l {xpr_lst(x)};
    while (tokens.front() != ")") {l->emplace_back(tokens_xpr(tokens));}
    tokens.pop_front();
    return x;
  }
  else if (token == ")") {throw std::runtime_error("unexpected ')'");}
  else {return xpr{token_atom(token)};}
}

xpr read(std::string const& str) {
  auto tokens = str_tokens(str);
  return tokens_xpr(tokens);
}

xpr eval(xpr& xr, env& ev) {
  if (auto const a = std::get_if<atm>(&xr)) {
    if (auto const s = std::get_if<sym>(a)) {if (ev.find(*s) != ev.end()) {return ev[*s];} throw std::runtime_error("unbound symbol");}
    if (auto const n = std::get_if<num>(a)) {
      if (auto const i = std::get_if<i64>(n)) {return i64_xpr(*i);}
      if (auto const f = std::get_if<f64>(n)) {return f64_xpr(*f);}
    }
    throw std::runtime_error("unknown atom");
  }
  if (auto const l = std::get_if<lst>(&xr)) {
    if (l->empty()) {return sym_xpr("nil");}
    xpr& fn = l->front();
    if (auto const a = std::get_if<atm>(&fn)) {
      if (auto const s = std::get_if<sym>(a)) {
        if (*s == "if") {
          xpr& cond {*std::next(l->begin(), 1)};
          xpr res {eval(cond, ev)};
          if (auto const s = std::get_if<sym>(std::get_if<atm>(&res))) {
            if (*s == "#f") {return eval(*std::next(l->begin(), 3), ev);}
          }
          else if (auto const a = std::get_if<lst>(&res)) {
            if (l->empty()) {return eval(*std::next(l->begin(), 3), ev);}
          }
          return eval(*std::next(l->begin(), 2), ev);
        }
        if (*s == "define") {
          xpr& symbol {*std::next(l->begin(), 1)};
          if (auto const s = std::get_if<sym>(std::get_if<atm>(&symbol))) {
            ev[*s] = eval(*std::next(l->begin(), 2), ev);
            return symbol;
          }
          throw std::runtime_error("expected symbol");
        }
        if (*s == "quote") {
          return l->back();
        }
        if (*s == "set!") {
          xpr& symbol {*std::next(l->begin(), 1)};
          if (auto const s = std::get_if<sym>(std::get_if<atm>(&symbol))) {
            if (ev.find(*s) != ev.end()) {ev[*s] = eval(*std::next(l->begin(), 2), ev);}
            return symbol;
          }
          return {};
        }
        if (*s == "lambda") {
          return xpr{[args = *std::next(l->begin(), 1), body = *std::next(l->begin(), 2), ep = ev, &ev](xpr xr) mutable -> xpr {
            auto p = std::get<lst>(args).begin();
            if (std::get<lst>(args).size() != std::get<lst>(xr).size()) {throw std::runtime_error("unbalanced lambda arguments");}
            env en {ev};
            for (auto& [k, v] : ep) {en[k] = v;}
            for (auto& e : std::get<lst>(xr)) {
              en[std::get<sym>(std::get<atm>(*p))] = e;
              std::advance(p, 1);
            }
            return eval(body, en);
          }};
        }
        xpr v {sym_xpr(*s)};
        xpr func {eval(v, ev)};
        if (auto const f = std::get_if<fun>(&func)) {
          xpr args;
          auto& argp = std::get<lst>(args);
          for (auto i = std::next(l->begin(), 1); i != l->end(); std::advance(i, 1)) {
            argp.emplace_back(eval(*i, ev));
          }
          return (*f)(args);
        }
      }
    }
    throw std::runtime_error("unknown function");
  }
  throw std::runtime_error("invalid eval");
}

template<typename T> std::string print(T const& t) {
  std::ostringstream os;
  os << "[";
  for_each(t,
    [&](auto const& e) {os << "'" << e << "', ";},
    [&](auto const& e) {os << "'" << e << "'";});
  os << "]\n";
  return os.str();
}

std::string print(xpr const& x) {
  std::ostringstream os;
  std::function<void(xpr const&)> const to_string_impl = [&](xpr const& x) {
    if (auto const a = std::get_if<atm>(&x)) {
      if (auto const s = std::get_if<sym>(a)) {os << *s;}
      else if (auto const n = std::get_if<num>(a)) {
        if (auto const i = std::get_if<i64>(n)) {os << *i;}
        else if (auto const f = std::get_if<f64>(n)) {os << *f;}
      }
      return;
    }
    lst const& l = std::get<lst>(x);
    os << "(";
    for_each(l,
      [&](auto const& e) {to_string_impl(e); os << " ";},
      [&](auto const& e) {to_string_impl(e);});
    os << ")";
  };
  to_string_impl(x);
  return os.str();
}

void repl() {
  env ev;
  env_init(ev);
  std::string input;
  std::cout << "Welcome to lispp\n> ";
  while (std::getline(std::cin, input)) {
    if (input.size()) {
      try {
        xpr x = read(input);
        xpr v = eval(x, ev);
        std::cout << print(v) << "\n";
      }
      catch (std::exception const& e) {std::cout << "Error: " << e.what() << "\n";}
    }
    std::cout << "> ";
  }
  std::cout << "\nttfn\n";
}

int main(int argc, char** argv) {
  repl();
  return 0;
}
