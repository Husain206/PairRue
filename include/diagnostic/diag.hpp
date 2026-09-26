#pragma once
#include "../lexer/span.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>

#define MAX_ERR_COUNT 100

enum class SeverityKind {
  Err,
  Warn,
  Note,
};

struct DiagInfo {
  SeverityKind severity = SeverityKind::Warn;
  Span span{};
  string err_msg = "";
  string note = "";
};

struct Diag {
  string filename;
  strview src;
  vec<DiagInfo> diags;
  size_t err_count = 0;

  explicit Diag(strview filename, strview src) : filename(filename), src(src) {}

  auto push_diag(DiagInfo diagInfo) -> void {
    if (diags.size() > MAX_ERR_COUNT) {
      render_all();
      exit(EXIT_FAILURE);
    }
    diags.push_back(diagInfo);
    if (diagInfo.severity == SeverityKind::Err)
      err_count++;
  }

  [[nodiscard]] bool has_error() const noexcept { return err_count > 0; }
  [[nodiscard]] usize error_count() const noexcept { return err_count; }

  auto render_all() -> void {
    for (const auto &d : diags) {
      render_single(d);
    }
  }

  auto render_single(const DiagInfo &d) -> void {
    const char *color = "\033[31m"; // red
    const char *label = "error";

    if (d.severity == SeverityKind::Warn) {
      color = "\033[33m"; // yellow
      label = "warning";
    } else if (d.severity == SeverityKind::Note) {
      color = "\033[36m"; // cyan
      label = "note";
    }

    std::cerr << "\033[1m" << filename << ":" << d.span.loc.line << ":"
              << d.span.loc.col << ": " << color << label << ": "
              << "\033[1m\033[37m" << d.err_msg << "\033[0m\n";

    // "╭", "╮", "╰", "╯", "─", "│"
    // "╔", "╗", "╚", "╝", "═", "║"

    if (d.span.start) {
      string src_line = (string)extract_line(d.span);
      if (!src_line.empty()) {
        std::cerr << "  │\n";
        std::cerr << d.span.loc.line << " │ " << src_line << "\n";

        std::cerr << "  │";

        std::cerr << color;
        for (u32 i = 1; i < d.span.loc.col; ++i) {
          std::cerr << " ";
        }
        std::cerr << " ┬";
        std::cerr << "\n  ╰─";
        for (u32 i = 1; i < d.span.loc.col; ++i) {
          std::cerr << "─";
        }
        std::cerr << "╯";
        std::cerr << "\033[0m";
      }
    }

    if (!d.note.empty()) {
      std::cerr << " = note: " << d.note << "\n";
    }
    std::cerr << "\n";
  }
  std::string_view extract_line(Span span) const {
    if (!span.start)
      return {};

    const char *src_start = src.data();
    const char *src_end = src.data() + src.size();

    if (span.start < src_start || span.start >= src_end)
      return {};

    const char *line_start = span.start;
    while (line_start > src_start && *(line_start - 1) != '\n') {
      line_start--;
    }

    const char *line_end = span.start;
    while (line_end < src_end && *line_end != '\n' && *line_end != '\r') {
      line_end++;
    }

    return std::string_view(line_start,
                            static_cast<size_t>(line_end - line_start));
  }
};
