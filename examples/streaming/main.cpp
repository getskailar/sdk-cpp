#include <skailar/skailar.hpp>

#include <cstdlib>
#include <iostream>

int main() {
    try {
        skailar::Client client;

        skailar::ChatCompletionRequest request;
        request.model = std::string {skailar::models::claude_sonnet_4_6};
        request.messages.push_back(
            skailar::user_message("Count from 1 to 5, one number per line."));

        auto stream = client.chat().completions().create_stream(request);
        while (auto chunk = stream->next()) {
            if (auto text = chunk->content_delta()) {
                std::cout << *text << std::flush;
            }
        }
        std::cout << "\n";
    } catch (const skailar::Error& error) {
        std::cerr << "skailar error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
