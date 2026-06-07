#include <skailar/skailar.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    try {
        skailar::Client client;

        skailar::SpeechRequest request;
        request.input = "Hello from the Skailar C++ SDK.";
        request.voice = skailar::Voice::Nova;

        const std::string audio = client.audio().speech().create(request);

        const char* path = "speech.mp3";
        std::ofstream out(path, std::ios::binary);
        out.write(audio.data(), static_cast<std::streamsize>(audio.size()));
        out.close();

        std::cout << "wrote " << audio.size() << " bytes to " << path << "\n";
    } catch (const skailar::Error& error) {
        std::cerr << "skailar error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
