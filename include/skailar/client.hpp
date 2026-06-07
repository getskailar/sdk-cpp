#ifndef SKAILAR_CLIENT_HPP
#define SKAILAR_CLIENT_HPP

#include <skailar/audio.hpp>
#include <skailar/chat.hpp>
#include <skailar/config.hpp>
#include <skailar/images.hpp>
#include <skailar/models.hpp>
#include <skailar/ping.hpp>
#include <skailar/streaming.hpp>
#include <skailar/uploads.hpp>

#include <memory>
#include <string>

namespace skailar {

class Client;

/// The chat-completions endpoint, reached through @ref ChatResource::completions.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.chat().completions()`
/// after move.
class ChatCompletionsResource {
public:
    /// Creates a non-streamed chat completion.
    ///
    /// This is a billable, side-effecting call and is not retried on 5xx.
    /// @throws skailar::Error on any failure.
    ChatCompletionResponse create(const ChatCompletionRequest& request) const;

    /// Creates a streamed chat completion, forcing stream mode on the wire.
    ///
    /// The returned stream owns the connection; let it go out of scope or call
    /// @ref ChatCompletionStream::close to release it. This is a billable,
    /// side-effecting call and is not retried.
    /// @throws skailar::Error on any failure establishing the stream.
    std::unique_ptr<ChatCompletionStream> create_stream(const ChatCompletionRequest& request) const;

private:
    friend class ChatResource;
    explicit ChatCompletionsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The chat resource, reached through @ref Client::chat.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.chat()` after move.
class ChatResource {
public:
    /// Returns the chat-completions sub-resource.
    ChatCompletionsResource completions() const noexcept {
        return ChatCompletionsResource(client_);
    }

private:
    friend class Client;
    explicit ChatResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The model-catalog resource, reached through @ref Client::models.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.models()` after move.
class ModelsResource {
public:
    /// Lists every model the gateway can route to.
    /// @throws skailar::Error on any failure.
    ModelList list() const;

    /// Retrieves the full detail card for a model. The id may contain slashes
    /// (for example "google/gemini-2.5-pro").
    /// @throws skailar::Error on any failure.
    Model retrieve(const std::string& id) const;

private:
    friend class Client;
    explicit ModelsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The image-generation resource, reached through @ref Client::images.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.images()` after move.
class ImagesResource {
public:
    /// Generates images from a prompt. Side-effecting; not retried on 5xx.
    /// @throws skailar::Error on any failure.
    ImageGenerationResponse generate(const ImageGenerationRequest& request) const;

private:
    friend class Client;
    explicit ImagesResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The transcription resource, reached through @ref AudioResource::transcriptions.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one through `client.audio()` after move.
class TranscriptionsResource {
public:
    /// Transcribes base64-encoded audio to text. Side-effecting; not retried on
    /// 5xx.
    /// @throws skailar::Error on any failure.
    TranscriptionResponse create(const TranscriptionRequest& request) const;

private:
    friend class AudioResource;
    explicit TranscriptionsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The speech-synthesis resource, reached through @ref AudioResource::speech.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one through `client.audio()` after move.
class SpeechResource {
public:
    /// Synthesizes speech and returns the full MP3 (audio/mpeg) clip as bytes.
    /// Side-effecting; not retried on 5xx.
    /// @throws skailar::Error on any failure.
    std::string create(const SpeechRequest& request) const;

private:
    friend class AudioResource;
    explicit SpeechResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The audio resource, reached through @ref Client::audio.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.audio()` after move.
class AudioResource {
public:
    /// Returns the speech-to-text sub-resource.
    TranscriptionsResource transcriptions() const noexcept {
        return TranscriptionsResource(client_);
    }

    /// Returns the text-to-speech sub-resource.
    SpeechResource speech() const noexcept { return SpeechResource(client_); }

private:
    friend class Client;
    explicit AudioResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The image-uploads resource, reached through @ref UploadsResource::images.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one through `client.uploads()` after move.
class ImageUploadsResource {
public:
    /// Uploads a base64-encoded image (without a `data:` prefix) and returns its
    /// stored URL, ready to embed as a vision input. Side-effecting; not retried
    /// on 5xx.
    /// @throws skailar::Error on any failure.
    UploadResponse create(const std::string& base64_data, ImageContentType content_type) const;

private:
    friend class UploadsResource;
    explicit ImageUploadsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The document-uploads resource, reached through @ref UploadsResource::files.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one through `client.uploads()` after move.
class FileUploadsResource {
public:
    /// Uploads a base64-encoded document (without a `data:` prefix) and returns
    /// its stored URL. Side-effecting; not retried on 5xx.
    /// @throws skailar::Error on any failure.
    UploadResponse create(const std::string& base64_data, FileContentType content_type) const;

private:
    friend class UploadsResource;
    explicit FileUploadsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The storage-uploads resource, reached through @ref Client::uploads.
///
/// Non-owning view of the parent Client. Does not survive a move of the
/// originating Client; obtain a fresh one with `client.uploads()` after move.
class UploadsResource {
public:
    /// Returns the image-uploads sub-resource.
    ImageUploadsResource images() const noexcept { return ImageUploadsResource(client_); }

    /// Returns the document-uploads sub-resource.
    FileUploadsResource files() const noexcept { return FileUploadsResource(client_); }

private:
    friend class Client;
    explicit UploadsResource(Client* client) noexcept : client_(client) { }

    Client* client_;
};

/// The entry point to the Skailar API. Construct it once and reuse it.
///
/// Once constructed, a Client is safe for concurrent use by multiple threads:
/// all of its state is immutable, and the underlying libcurl handles are guarded
/// by a curl share with locking. Obtain resource handles with @ref chat,
/// @ref models, @ref images, @ref audio, and @ref uploads.
///
/// A Client is movable but not copyable. Resource handles obtained before a move
/// are invalidated by it; obtain fresh ones from the moved-to Client.
class Client {
public:
    /// Constructs a Client from the `SKAILAR_API_KEY` and `SKAILAR_BASE_URL`
    /// environment variables.
    /// @throws skailar::Error of @ref ErrorKind::Config when no API key is set.
    Client();

    /// Constructs a Client from an explicit configuration.
    /// @throws skailar::Error of @ref ErrorKind::Config when the API key is
    ///         missing or the base URL is malformed.
    explicit Client(ClientConfig config);

    ~Client();

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /// Returns the chat resource.
    ChatResource chat() noexcept { return ChatResource(this); }
    /// Returns the model-catalog resource.
    ModelsResource models() noexcept { return ModelsResource(this); }
    /// Returns the image-generation resource.
    ImagesResource images() noexcept { return ImagesResource(this); }
    /// Returns the audio resource.
    AudioResource audio() noexcept { return AudioResource(this); }
    /// Returns the storage-uploads resource.
    UploadsResource uploads() noexcept { return UploadsResource(this); }

    /// Verifies the API key against the gateway.
    /// @throws skailar::Error on any failure (for example @ref ErrorKind::Auth).
    PingKeyResponse ping();

    /// @cond INTERNAL
    /// Internal implementation handle, used by the resource classes.
    class Impl;
    Impl* impl() noexcept { return impl_.get(); }
    /// @endcond

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace skailar

#endif // SKAILAR_CLIENT_HPP
