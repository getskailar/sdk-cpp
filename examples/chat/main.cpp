#include <skailar/skailar.hpp>

#include <cstdlib>
#include <iostream>

int main() {
    try {
        // Reads SKAILAR_API_KEY (and optionally SKAILAR_BASE_URL) from the env.
        skailar::Client client;

        skailar::ChatCompletionRequest request;
        request.model = std::string {skailar::models::claude_sonnet_4_6};
        request.messages.push_back(skailar::user_message("In one sentence, what is Skailar?"));

        const skailar::ChatCompletionResponse response
            = client.chat().completions().create(request);
        std::cout << response.choices.at(0).message.content.text() << "\n";
    } catch (const skailar::Error& error) {
        std::cerr << "skailar error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
