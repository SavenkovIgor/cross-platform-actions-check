// Cross-platform smoke test for std::expected and std::filesystem.
// Exits with 0 when every check passes, 1 otherwise.

#include <chrono>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::cout << (ok ? "[ OK ] " : "[FAIL] ") << what << '\n';
    if (!ok) ++failures;
}

// ---------- std::expected ----------

enum class ParseError { Empty, NotANumber };

std::expected<int, ParseError> parse_int(const std::string& s) {
    if (s.empty()) return std::unexpected(ParseError::Empty);
    try {
        std::size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return std::unexpected(ParseError::NotANumber);
        return v;
    } catch (...) {
        return std::unexpected(ParseError::NotANumber);
    }
}

void test_expected() {
    auto ok = parse_int("42");
    check(ok.has_value() && *ok == 42, "expected: value on success");

    auto empty = parse_int("");
    check(!empty && empty.error() == ParseError::Empty, "expected: error on empty input");

    auto bad = parse_int("12x");
    check(!bad && bad.error() == ParseError::NotANumber, "expected: error on garbage input");

    auto doubled = parse_int("21").transform([](int v) { return v * 2; });
    check(doubled && *doubled == 42, "expected: transform");

    auto chained = parse_int("5").and_then([](int v) -> std::expected<int, ParseError> {
        if (v > 3) return std::unexpected(ParseError::NotANumber);
        return v;
    });
    check(!chained && chained.error() == ParseError::NotANumber, "expected: and_then propagates error");

    auto recovered = parse_int("nope").or_else(
        [](ParseError) -> std::expected<int, ParseError> { return -1; });
    check(recovered && *recovered == -1, "expected: or_else recovers");

    check(parse_int("zzz").value_or(7) == 7, "expected: value_or fallback");
}

// ---------- std::filesystem (errors reported through std::expected) ----------

std::expected<std::string, std::error_code> read_file(const fs::path& p) {
    std::error_code ec;
    if (!fs::exists(p, ec)) {
        return std::unexpected(ec ? ec : std::make_error_code(std::errc::no_such_file_or_directory));
    }
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::unexpected(std::make_error_code(std::errc::io_error));
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

fs::path make_unique_temp_dir() {
    std::error_code ec;
    const fs::path base = fs::temp_directory_path(ec);
    if (ec) return {};
    // Process-unique name; filesystem::path handles separators for us.
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path dir = base / ("xplat_demo_" + std::to_string(stamp));
    return fs::create_directories(dir, ec) && !ec ? dir : fs::path{};
}

void test_filesystem() {
    const fs::path root = make_unique_temp_dir();
    check(!root.empty() && fs::is_directory(root), "filesystem: temp directory created");
    if (root.empty()) return;

    // Nested directories + file write/read round-trip.
    const fs::path nested = root / "a" / "b";
    std::error_code ec;
    check(fs::create_directories(nested, ec) && !ec, "filesystem: create_directories");

    const fs::path file = nested / "hello.txt";
    {
        std::ofstream out(file, std::ios::binary);
        out << "hello\nworld";
    }
    check(fs::is_regular_file(file), "filesystem: file exists");
    check(fs::file_size(file) == 11, "filesystem: file_size");

    auto content = read_file(file);
    check(content && *content == "hello\nworld", "filesystem+expected: read_file round-trip");

    auto missing = read_file(root / "missing.txt");
    check(!missing && missing.error() == std::errc::no_such_file_or_directory,
          "filesystem+expected: missing file yields error");

    // Path manipulation (compare generic_string so separators don't matter).
    check(file.filename() == "hello.txt", "path: filename");
    check(file.stem() == "hello", "path: stem");
    check(file.extension() == ".txt", "path: extension");
    check(fs::path(file).replace_extension(".md").extension() == ".md", "path: replace_extension");
    check((root / "a" / ".." / "a" / "b").lexically_normal().generic_string() ==
              (root / "a" / "b").lexically_normal().generic_string(),
          "path: lexically_normal");

    // Rename + directory iteration.
    const fs::path renamed = nested / "renamed.txt";
    fs::rename(file, renamed, ec);
    check(!ec && fs::exists(renamed) && !fs::exists(file), "filesystem: rename");

    std::size_t files = 0;
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (e.is_regular_file()) ++files;
    }
    check(files == 1, "filesystem: recursive_directory_iterator");

    // Cleanup.
    const auto removed = fs::remove_all(root, ec);
    check(!ec && removed >= 3 && !fs::exists(root), "filesystem: remove_all cleanup");
}

}  // namespace

int main() {
    std::cout << "__cplusplus = " << __cplusplus << '\n';
#ifdef _LIBCPP_VERSION
    std::cout << "stdlib: libc++ " << _LIBCPP_VERSION << '\n';
#elif defined(_MSC_VER)
    std::cout << "stdlib: MSVC STL (_MSC_VER " << _MSC_VER << ")\n";
#elif defined(__GLIBCXX__)
    std::cout << "stdlib: libstdc++ " << __GLIBCXX__ << '\n';
#endif
#ifdef __clang_version__
    std::cout << "clang: " << __clang_version__ << '\n';
#endif

    test_expected();
    test_filesystem();

    if (failures != 0) {
        std::cout << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All checks passed\n";
    return EXIT_SUCCESS;
}
