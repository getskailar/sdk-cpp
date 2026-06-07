# Skailar C++ SDK

The official C++ SDK for the [Skailar](https://skailar.com) API — a
multi-provider LLM gateway with an OpenAI-compatible surface. One client talks
to chat completions, model discovery, image generation, speech synthesis and
transcription, and storage uploads across Anthropic, OpenAI, Google, DeepSeek,
and xAI models, billed per request from the Skailar account that owns the key.

Idiomatic, modern C++17. Exceptions for errors, RAII for resources, explicit
ownership, no global state. Depends only on libcurl and nlohmann/json.

> Status: pre-release `v0.0.1`. The API may change before 1.0.

## Installation

The SDK needs a C++17 compiler, [libcurl](https://curl.se/libcurl/) (≥ 7.68),
and [nlohmann/json](https://github.com/nlohmann/json) (≥ 3.10).

### vcpkg

The SDK is distributed from its own vcpkg registry. Add it to your project's
`vcpkg-configuration.json` once:

```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg",
    "baseline": "<your usual baseline>"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/getskailar/sdk-cpp",
      "baseline": "<latest commit on main>",
      "packages": ["skailar"]
    }
  ]
}
```

Then install it like any other port. The port is **static-only**, so use a
static triplet on Windows:

```sh
vcpkg install skailar                      # Linux / macOS (static by default)
vcpkg install skailar:x64-windows-static   # Windows
```

Set the `baseline` of the Skailar registry to the latest commit SHA of `main`.

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  skailar
  GIT_REPOSITORY https://github.com/getskailar/sdk-cpp.git
  GIT_TAG v0.0.1)
FetchContent_MakeAvailable(skailar)

target_link_libraries(my_app PRIVATE Skailar::skailar)
```

`find_package(CURL)` and `find_package(nlohmann_json)` must be satisfiable in
your build (via vcpkg, Conan, or system packages).

### System install

```sh
git clone https://github.com/getskailar/sdk-cpp.git
cd sdk-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

## Quickstart

```cpp
#include <iostream>
#include <skailar/skailar.hpp>

int main()
{
    // Reads SKAILAR_API_KEY (and optionally SKAILAR_BASE_URL) from the env.
    skailar::Client client;

    skailar::ChatCompletionRequest request;
    request.model = std::string { skailar::models::claude_sonnet_4_6 };
    request.messages.push_back(skailar::user_message("Hello!"));

    const auto response = client.chat().completions().create(request);
    std::cout << response.choices.at(0).message.content.text() << "\n";
}
```

Construct request structs field by field and use the message helpers
(`user_message`, `system_message`, `assistant_message`, `tool_message`) to keep
call sites short.

## CMake integration

Once installed (or available via FetchContent), consume the package with:

```cmake
find_package(skailar CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Skailar::skailar)
```

The umbrella header is `<skailar/skailar.hpp>`; the individual headers
(`<skailar/client.hpp>`, `<skailar/chat.hpp>`, …) are also self-contained.

## Streaming

`create_stream` returns a `std::unique_ptr<skailar::ChatCompletionStream>`. Pull
chunks with `next()` until it returns `std::nullopt`, which signals a clean end
of stream. A mid-stream failure throws `skailar::Error`. The stream's destructor
closes the underlying connection.

```cpp
auto stream = client.chat().completions().create_stream(request);
while (auto chunk = stream->next()) {
    if (auto text = chunk->content_delta()) {
        std::cout << *text << std::flush;
    }
}
```

The pull shape composes well with range-style loops and never forces nested
callbacks.

## Configuration

Pass a `ClientConfig` for explicit control:

```cpp
skailar::ClientConfig config;
config.api_key = "skl_live_...";
config.base_url = "http://localhost:8080";
config.timeout = std::chrono::seconds(30);
config.max_retries = 2;
config.default_headers["x-trace-id"] = "abc123";

skailar::Client client(config);
```

| Field             | Default                              |
| ----------------- | ------------------------------------ |
| `api_key`         | `$SKAILAR_API_KEY`                   |
| `base_url`        | `$SKAILAR_BASE_URL` or `https://api.skailar.com` |
| `timeout`         | 60 seconds                           |
| `max_retries`     | 2 (three attempts total)             |
| `default_headers` | none                                 |

The `Authorization` header is owned by the SDK and cannot be overridden through
`default_headers`. A missing API key or a malformed base URL throws
`skailar::Error` with `ErrorKind::Config` at construction.

`skailar::Client` is safe to share across threads once constructed; it is
movable but not copyable. Resource handles such as `client.chat()` are
lightweight non-owning views — obtain a fresh one after moving the client.

## Error handling

Every fallible call throws a single `skailar::Error`. Branch on `kind()`:

```cpp
try {
    auto response = client.chat().completions().create(request);
} catch (const skailar::Error& e) {
    switch (e.kind()) {
    case skailar::ErrorKind::RateLimit:
        std::this_thread::sleep_for(std::chrono::seconds(e.retry_after()));
        break;
    case skailar::ErrorKind::Auth:
        std::cerr << "check your API key\n";
        break;
    default:
        std::cerr << e.what() << " (status " << e.status() << ")\n";
    }
}
```

`Error` exposes `kind()`, `status()`, `code()`, `message()`, `request_id()`,
`raw()`, and `retry_after()`. Retries are automatic per the policy below; the
exception surfaces only after retries are exhausted.

### Retry policy

- Retried: `429` (always), `5xx` on idempotent `GET`, and transient transport
  errors / timeouts on `GET`.
- Not retried: side-effecting `POST` requests (chat, images, audio, uploads) on
  `5xx`, to avoid double billing.
- Backoff is exponential with full jitter (base 500 ms, cap 8 s). A
  `Retry-After` header takes precedence and is capped at 60 s; the raw value is
  reported on `Error::retry_after()`.

## Drop-in OpenAI alternative

The Skailar wire format mirrors the OpenAI API, so the request and response
shapes here match what OpenAI-compatible code expects. There is no official
OpenAI C++ SDK; if you currently call the API with raw libcurl or a community
wrapper, you can migrate by pointing `base_url` at Skailar and using these typed
request/response structs.

## Building from source

```sh
# Configure with the vcpkg toolchain (requires VCPKG_ROOT) and tests/examples.
cmake --preset=default

# Or, with dependencies already on the system:
cmake -S . -B build -DSKAILAR_BUILD_TESTS=ON -DSKAILAR_BUILD_EXAMPLES=ON

cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Build options: `SKAILAR_BUILD_TESTS` (default `OFF`), `SKAILAR_BUILD_EXAMPLES`
(default `OFF`), `SKAILAR_INSTALL` (default `ON`), `SKAILAR_WERROR` (default
`ON`). The library builds as a static archive by default; set
`-DBUILD_SHARED_LIBS=ON` for a shared library.

## Status

Pre-release `v0.0.1`. Covered endpoints: chat completions (JSON + streaming),
model list/retrieve, image generation, audio transcription and speech, image
and file uploads, and key verification.

## License

MIT. See [LICENSE](LICENSE).
