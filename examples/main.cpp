#include <print>
#include <chrono>
#include <thread>
#include "termml.hpp"
#include "termml/core/commands.hpp"
#include "termml/core/event.hpp"
#include "termml/core/raw_mode.hpp"
#include "termml/xml/lexer.hpp"
#include "termml/xml/parser.hpp"
#include "termml/layout.hpp"

using namespace termml;

void sleep_frame(std::chrono::steady_clock::time_point start, int ms = 16) {
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto delay = ms - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    if (delay > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

int main() {
    std::string_view source = R"(
    <row color="red">
        <b min-width="30%">
            ⚠️ Warning:
        </b>
        <text>Disk space is almost full.</text>
    </row>
    test
    <col>
        <text>Usage:</text>
        <b color="#ff5555">95%</b>
    </col>
)";
    auto l = xml::Lexer(source, "unknown");
    l.lex();
    auto parser = xml::Parser(std::move(l));
    parser.parse();
    auto layout = layout::LayoutContext({ .x = 0, .y = 0, .width = 100, .height = 100 });
    layout.compute(parser.context.get());
    parser.context->dump_xml();
    // layout.dump(parser.context.get());

    return 0;
}

int main2() {
    std::println("Hey");
    core::RawModeGuard r{};
    auto& cmd = core::Command::out();

    auto x = 0;
    auto dx = 1;

    auto window = WindowSize(30, 50);
    cmd.hide_cursor();

    while (true) {
        auto start = std::chrono::steady_clock::now();
        auto event = Event::parse(r);
        if (event.is<TerminateEvent>()) break;
        if (event.is<KeyboardEvent>()) {
            auto e = event.as<KeyboardEvent>();
            auto txt = e.str();
            if (txt == "q" || e.key == KeyboardKey::Escape) {
                break;
            }
        }
        x += dx;
        if (x >= static_cast<int>(window.cols)) {
            dx *= -1;
            x = static_cast<int>(window.cols);
        } else if (x <= 0) {
            dx *= -1;
            x = 0;
        }

        sleep_frame(start, 16);
        cmd.clear_line();
        cmd.write("a");
        cmd.move(static_cast<unsigned>(x), 0u);
    }

    cmd.hide_cursor(false);
    std::println("Hey");
    return 0;
}
