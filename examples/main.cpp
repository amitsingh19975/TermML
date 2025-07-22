#include <print>
#include <chrono>
#include <thread>
#include "termml.hpp"

using namespace termml;

void sleep_frame(std::chrono::steady_clock::time_point start, int ms = 16) {
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto delay = ms - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    if (delay > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

int main() {
    std::string_view source = R"(
    <row color="red" border="thin solid red">
        <b min-width="30%">
            ⚠️ Warnin Lorem Ipsum is simply dummy\n text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.g:
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
    auto layout = layout::LayoutContext({ .x = 0, .y = 0, .width = 50, .height = 50 });
    layout.compute(parser.context.get());
    // parser.context->dump_xml();
    // layout.dump(parser.context.get());

    auto terminal = core::Terminal(50, 50);
    auto device = core::Device(&terminal);
    auto& cmd = core::Command::out();

    layout.render(device, parser.context.get());
    device.flush(cmd, 0, 0);

    
    // text::TextRenderer t {
    //     .text = "Lorem Ipsum is simply dummy\n text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.",
    //     .container = { .width = 20, .height = 100 },
    // };
    //
    // auto style = parser.context->styles[0];
    // t.container.height = t.measure_height(style);
    //
    // // style.whitespace = style::Whitespace::PreLine;
    // t.render(device, style);
    // //
    // cmd.clear_screen();
    // device.flush(cmd, 0, 0);

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
