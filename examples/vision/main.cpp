#include <skailar/skailar.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    try {
        skailar::Client client;

        std::vector<skailar::ContentPart> parts;
        parts.push_back(skailar::text_part("What is in this image?"));
        parts.push_back(
            skailar::image_part("https://upload.wikimedia.org/wikipedia/commons/thumb/"
                                "d/dd/Gfp-wisconsin-madison-the-nature-boardwalk.jpg/"
                                "640px-Gfp-wisconsin-madison-the-nature-boardwalk.jpg"));

        skailar::ChatMessage message;
        message.role = skailar::Role::User;
        message.content = skailar::MessageContent::parts(std::move(parts));

        skailar::ChatCompletionRequest request;
        request.model = std::string {skailar::models::gpt_5};
        request.messages.push_back(std::move(message));

        const skailar::ChatCompletionResponse response
            = client.chat().completions().create(request);
        std::cout << response.choices.at(0).message.content.text() << "\n";
    } catch (const skailar::Error& error) {
        std::cerr << "skailar error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
