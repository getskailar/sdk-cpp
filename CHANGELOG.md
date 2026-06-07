# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.0.1] - 2026-06-07

Initial pre-release of the official Skailar SDK for C++.

### Added

- `skailar::Client` configured by a `ClientConfig` struct (`api_key`,
  `base_url`, `timeout`, `max_retries`, `default_headers`); movable, not
  copyable, and safe for concurrent use once constructed.
- Chat completions: `client.chat().completions().create` (JSON) and
  `create_stream` (Server-Sent Events).
- Model catalog: `client.models().list` and `client.models().retrieve`.
- Image generation: `client.images().generate`.
- Audio: `client.audio().transcriptions().create` and
  `client.audio().speech().create` (returns the full `audio/mpeg` clip as
  `std::string` bytes).
- Storage uploads: `client.uploads().images().create` and
  `client.uploads().files().create`.
- Key verification: `client.ping`.
- A single concrete `skailar::Error` exception type with an `ErrorKind`
  discriminant, HTTP status, machine-readable code, request id, raw body, and
  uncapped `retry_after`.
- `ChatCompletionStream` with a pull-based `next()` returning
  `std::optional<ChatCompletionChunk>`, backed by a dependency-light SSE parser.
- Known model-id constants in `<skailar/model_ids.hpp>` and free helper
  functions (`user_message`, `system_message`, `assistant_message`,
  `tool_message`, `text_part`, `image_part`, `function_tool`).
- CMake package config for `find_package(skailar CONFIG REQUIRED)` exporting the
  `Skailar::skailar` target, plus a vcpkg manifest for development.

### Fixed

Shipped already-corrected for bugs fixed in the sister SDKs (TypeScript through
Go 0.0.1), so the C++ SDK never regressed through them:

- The retry loop is bounded by `max_retries` and never waits on an unbounded
  timer; cancellation is reserved for a future `CancelToken` (C++ v0 exposes
  none).
- Internal timeouts are reported as `ErrorKind::Timeout`, distinct from
  `ErrorKind::Network` for other transport failures (mapped from the libcurl
  result code).
- The destructor of `ChatCompletionStream` closes the underlying libcurl
  connection instead of leaking it; `close()` is idempotent.
- The `Authorization` header cannot be overridden by `default_headers`;
  conflicting keys are dropped case-insensitively before the bearer token is
  applied.
- Side-effecting `POST` requests (chat completions, image generation, speech,
  transcription, uploads) are never retried on `5xx`, to avoid double billing.
  Only idempotent `GET` requests are retried on `5xx`.
- `Retry-After` is capped at 60 seconds; the uncapped server value is still
  exposed on `Error::retry_after`.
- The SSE parser accepts all three line terminators (`\n`, `\r\n`, `\r`),
  including a `\r\n` pair split across two network reads.
- Every I/O method documents `@throws skailar::Error`; only trivial accessors
  and move operations are `noexcept`.

[Unreleased]: https://github.com/getskailar/sdk-cpp/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/getskailar/sdk-cpp/releases/tag/v0.0.1
