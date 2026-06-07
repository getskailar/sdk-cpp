#ifndef SKAILAR_MODEL_IDS_HPP
#define SKAILAR_MODEL_IDS_HPP

#include <string_view>

namespace skailar {

/// Known model identifiers, provided for autocomplete. Any string is a valid
/// model; these constants are not exhaustive and the catalog may change. Call
/// @ref ModelsResource::list for the live list.
///
/// The `model` request field is a `std::string`; assign a constant directly, as
/// `req.model = std::string{skailar::models::claude_sonnet_4_6};`.
namespace models {

/// `claude-opus-4-8`.
inline constexpr std::string_view claude_opus_4_8 = "claude-opus-4-8";
/// `claude-opus-4-7`.
inline constexpr std::string_view claude_opus_4_7 = "claude-opus-4-7";
/// `claude-opus-4-6`.
inline constexpr std::string_view claude_opus_4_6 = "claude-opus-4-6";
/// `claude-sonnet-4-6`.
inline constexpr std::string_view claude_sonnet_4_6 = "claude-sonnet-4-6";
/// `claude-sonnet-4-5`.
inline constexpr std::string_view claude_sonnet_4_5 = "claude-sonnet-4-5";
/// `claude-haiku-4-5`.
inline constexpr std::string_view claude_haiku_4_5 = "claude-haiku-4-5";

/// `gpt-5.5`.
inline constexpr std::string_view gpt_5_5 = "gpt-5.5";
/// `gpt-5.4`.
inline constexpr std::string_view gpt_5_4 = "gpt-5.4";
/// `gpt-5.4-mini`.
inline constexpr std::string_view gpt_5_4_mini = "gpt-5.4-mini";
/// `gpt-5.4-nano`.
inline constexpr std::string_view gpt_5_4_nano = "gpt-5.4-nano";
/// `gpt-5.1`.
inline constexpr std::string_view gpt_5_1 = "gpt-5.1";
/// `gpt-5`.
inline constexpr std::string_view gpt_5 = "gpt-5";
/// `gpt-5-mini`.
inline constexpr std::string_view gpt_5_mini = "gpt-5-mini";

/// `o3`.
inline constexpr std::string_view o3 = "o3";
/// `o4-mini`.
inline constexpr std::string_view o4_mini = "o4-mini";

/// `gemini-3.5-flash`.
inline constexpr std::string_view gemini_3_5_flash = "gemini-3.5-flash";
/// `gemini-3.1-pro-preview`.
inline constexpr std::string_view gemini_3_1_pro_preview = "gemini-3.1-pro-preview";
/// `gemini-3-flash-preview`.
inline constexpr std::string_view gemini_3_flash_preview = "gemini-3-flash-preview";
/// `gemini-2.5-pro`.
inline constexpr std::string_view gemini_2_5_pro = "gemini-2.5-pro";
/// `gemini-2.5-flash`.
inline constexpr std::string_view gemini_2_5_flash = "gemini-2.5-flash";
/// `gemini-2.5-flash-lite`.
inline constexpr std::string_view gemini_2_5_flash_lite = "gemini-2.5-flash-lite";

/// `deepseek-v4-pro`.
inline constexpr std::string_view deepseek_v4_pro = "deepseek-v4-pro";
/// `deepseek-v4-flash`.
inline constexpr std::string_view deepseek_v4_flash = "deepseek-v4-flash";

/// `grok-4.3`.
inline constexpr std::string_view grok_4_3 = "grok-4.3";
/// `grok-4.20-reasoning`.
inline constexpr std::string_view grok_4_20_reasoning = "grok-4.20-reasoning";
/// `grok-4.20-non-reasoning`.
inline constexpr std::string_view grok_4_20_non_reasoning = "grok-4.20-non-reasoning";
/// `grok-build-0.1`.
inline constexpr std::string_view grok_build_0_1 = "grok-build-0.1";

/// `gpt-image-1`.
inline constexpr std::string_view gpt_image_1 = "gpt-image-1";

} // namespace models
} // namespace skailar

#endif // SKAILAR_MODEL_IDS_HPP
