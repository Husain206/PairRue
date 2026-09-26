#pragma once

#include "macros.hpp"
#include "types.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

struct Flags {
  string file;
  string filename;
  bool display_ast = false;
};

struct CmdParser {
  private:
    int argc_{0};
    char** argv_{nullptr};
    Flags flags_{};
  public:
    explicit CmdParser(int argc, char** argv) : argc_(argc), argv_(argv) {}

  private:
  [[nodiscard]] strview shift() noexcept {
    ASSERT(argc_ > 0 && "argc is 0");
    argc_--;
    return static_cast<strview>(*(argv_)++);
  }

  [[nodiscard]] string readfile(strview filename) const noexcept {
    std::ifstream ifs((static_cast<string>(filename)));
    ASSERT(ifs.is_open() && "couldn't open file");
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  [[nodiscard]] ccstr help() const noexcept {
    ccstr help = "-h --help\t displays this\n-f --file\t <filename> takes in a file\n--display-ast\t duh";
    return help;
  }

  [[nodiscard]] ccstr usage() const noexcept {
    return "Usage: ./exec <filename>\n";
  }

  public:
  [[nodiscard]] string file() const noexcept {  ASSERT(!flags_.file.empty() && "file flag is not set"); return flags_.file; }
  [[nodiscard]] string filename() const noexcept { ASSERT(!flags_.filename.empty() && "filename flag is not set"); return flags_.filename; }
  [[nodiscard]] bool display_ast() const noexcept { return flags_.display_ast; }

  void parse() noexcept {
    auto init = shift();
    if(argc_ == 0)
      PANIC(usage());

    while (argc_ > 0){
      auto flag = shift();

      if(flag == "-h" || flag == "--help"){
        std::cout << help();
        continue;
      }

      if(flag == "-f" || flag == "--file"){
        auto filename = shift();
        flags_.file = readfile(filename);
        flags_.filename = filename;
        continue;
      }

      if(flag == "--display-ast"){
        flags_.display_ast = true;
        continue;
      }

      PANIC("unknown flag");
    }
  }
};
