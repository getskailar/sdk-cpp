#include <skailar/skailar.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    try {
        skailar::Client client;

        nlohmann::json parameters = {
            {"type", "object"},
            {"properties", {{"city", {{"type", "string"}, {"description", "City name."}}}}},
            {"required", nlohmann::json::array({"city"})},
        };

        skailar::ChatCompletionRequest request;
        request.model = std::string {skailar::models::claude_sonnet_4_6};
        request.messages.push_back(skailar::user_message("What is the weather in Paris?"));
        request.tools = std::vector<skailar::Tool> {
            skailar::function_tool("get_weather", "Get the current weather for a city",
                                   std::move(parameters)),
        };
        request.tool_choice = skailar::ToolChoice::automatic();

        const skailar::ChatCompletionResponse response
            = client.chat().completions().create(request);
        const skailar::Choice& choice = response.choices.at(0);
        if (choice.message.tool_calls.has_value()) {
            for (const skailar::ToolCall& call : *choice.message.tool_calls) {
                std::cout << "tool: " << call.function.name << "\n";
                std::cout << "args: " << call.function.arguments << "\n";
            }
        } else {
            std::cout << choice.message.content.text() << "\n";
        }
    } catch (const skailar::Error& error) {
        std::cerr << "skailar error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
