// polza.hpp — header-only C++20 client for the Polza.ai API (libcurl + nlohmann/json).
//
//   polza::Client client({.api_key = std::getenv("POLZA_API_KEY")});
//   auto reply = client.chat_text("openai/gpt-4o", "Привет!");
//
// Everything else (streaming, tools, media, embeddings) is built on Client::post/get,
// which handle auth, retries and error decoding. See examples/.
#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace polza {

using json = nlohmann::json;

// ---------------------------------------------------------------- errors ---

/// Thrown for every non-2xx response and for transport failures.
/// `status` is the HTTP code (0 for transport errors), `code` is Polza's
/// machine-readable error.code when the body carried one.
class Error : public std::runtime_error {
public:
    Error(std::string message, long status, std::string code)
        : std::runtime_error(std::move(message))
        , status(status)
        , code(std::move(code)) {
    }

    long status;
    std::string code;

    /// 429 and 5xx are worth retrying; 4xx means the request itself is wrong.
    bool retryable() const {
        return status == 0 || status == 408 || status == 429 || status >= 500;
    }
    /// Balance exhausted — retrying will not help, top up the account.
    bool out_of_funds() const {
        return status == 402;
    }
};

// ----------------------------------------------------------------- usage ---

/// Flattened `usage` object. Every endpoint reports cost in roubles.
struct Usage {
    std::int64_t prompt_tokens = 0;
    std::int64_t completion_tokens = 0;
    std::int64_t total_tokens = 0;
    std::int64_t reasoning_tokens = 0;
    std::int64_t cached_tokens = 0;
    double cost_rub = 0.0;
};

inline Usage parse_usage(const json& response) {
    Usage usage;
    const auto it = response.find("usage");
    if (it == response.end() || !it->is_object()) {
        return usage;
    }
    const json& u = *it;
    // Responses API names them input_tokens/output_tokens; both spellings are sent.
    usage.prompt_tokens = u.value("prompt_tokens", u.value("input_tokens", std::int64_t { 0 }));
    usage.completion_tokens = u.value("completion_tokens", u.value("output_tokens", std::int64_t { 0 }));
    usage.total_tokens = u.value("total_tokens", std::int64_t { 0 });
    usage.cost_rub = u.value("cost_rub", u.value("cost", 0.0));
    if (const auto d = u.find("completion_tokens_details"); d != u.end() && d->is_object()) {
        usage.reasoning_tokens = d->value("reasoning_tokens", std::int64_t { 0 });
    }
    if (const auto d = u.find("prompt_tokens_details"); d != u.end() && d->is_object()) {
        usage.cached_tokens = d->value("cached_tokens", std::int64_t { 0 });
    }
    return usage;
}

// ------------------------------------------------------------ sse parser ---

/// Incremental parser for `text/event-stream` bodies. Fed arbitrary byte chunks,
/// it invokes the handler once per complete `data:` payload and never blocks on
/// partial lines — libcurl splits the stream at arbitrary offsets.
class SseParser {
public:
    /// Handler receives one payload. Return false to request cancellation.
    using Handler = std::function<bool(std::string_view payload)>;

    explicit SseParser(Handler handler)
        : _handler(std::move(handler)) {
    }

    /// Returns false once the handler asked to stop (or `[DONE]` was seen).
    bool feed(std::string_view chunk) {
        if (_finished) {
            return false;
        }
        _buffer.append(chunk);
        std::size_t start = 0;
        while (true) {
            const std::size_t newline = _buffer.find('\n', start);
            if (newline == std::string::npos) {
                break;
            }
            std::string_view line(_buffer.data() + start, newline - start);
            if (!line.empty() && line.back() == '\r') {
                line.remove_suffix(1);
            }
            start = newline + 1;
            if (!handle_line(line)) {
                _buffer.erase(0, start);
                _finished = true;
                return false;
            }
        }
        _buffer.erase(0, start);
        return true;
    }

    bool finished() const {
        return _finished;
    }

private:
    bool handle_line(std::string_view line) {
        if (line.empty() || line.front() == ':') {
            return true;  // keep-alive comment, e.g. ": processing"
        }
        if (!line.starts_with("data:")) {
            return true;  // event:/id:/retry: — not needed by either Polza stream format
        }
        line.remove_prefix(5);
        while (!line.empty() && line.front() == ' ') {
            line.remove_prefix(1);
        }
        if (line == "[DONE]") {
            return false;
        }
        return _handler(line);
    }

    Handler _handler;
    std::string _buffer;
    bool _finished = false;
};

// ---------------------------------------------------------------- client ---

struct Options {
    std::string api_key;
    /// Without the version segment: paths passed to post()/get() start with /v1 or /v2.
    std::string base_url = "https://polza.ai/api";
    /// Polza's own ceiling is 600 s; video generation really can take that long.
    long timeout_seconds = 600;
    long connect_timeout_seconds = 15;
    /// Retries apply to 429/5xx/transport errors only.
    int max_retries = 3;
    std::chrono::milliseconds initial_backoff { 500 };
    /// Sent as User-Agent; useful when asking support about a request.
    std::string user_agent = "polza-cpp/1.0";
    bool verbose = false;
};

class Client {
public:
    explicit Client(Options options)
        : _options(std::move(options)) {
        if (_options.api_key.empty()) {
            throw Error("Polza API key is empty", 0, "NO_API_KEY");
        }
        global_init();
    }

    const Options& options() const {
        return _options;
    }

    // -- raw verbs ----------------------------------------------------------

    /// POST a JSON body and decode the JSON reply. Retries transient failures.
    json post(std::string_view path, const json& body) const {
        return request(path, &body, "POST");
    }

    /// GET a JSON resource. `path` may already carry a query string.
    json get(std::string_view path) const {
        return request(path, nullptr, "GET");
    }

    /// PATCH with no body — used by /v1/storage/files/{id}/keep.
    json patch(std::string_view path) const {
        return request(path, nullptr, "PATCH");
    }

    // -- chat ---------------------------------------------------------------

    /// POST /v1/chat/completions. `body` is passed through untouched, so any
    /// documented field (tools, provider, plugins, reasoning...) just works.
    json chat(json body) const {
        return post("/v1/chat/completions", body);
    }

    /// One-shot helper: send a single user message, return the assistant text.
    std::string chat_text(std::string_view model, std::string_view prompt) const {
        json body = { { "model", model },
                      { "messages", json::array({ { { "role", "user" }, { "content", prompt } } }) } };
        return content_of(chat(std::move(body)));
    }

    /// Extracts choices[0].message.content, tolerating a null (tool-call-only) reply.
    static std::string content_of(const json& completion) {
        const auto choices = completion.find("choices");
        if (choices == completion.end() || !choices->is_array() || choices->empty()) {
            return {};
        }
        const json& message = (*choices)[0].value("message", json::object());
        const auto content = message.find("content");
        if (content == message.end() || content->is_null()) {
            return {};
        }
        return content->is_string() ? content->get<std::string>() : content->dump();
    }

    /// Streaming chat. `on_chunk` receives each parsed SSE payload as JSON;
    /// return false from it to abort the request early. `stream: true` is set
    /// automatically. Streaming responses are never retried — a half-delivered
    /// answer must not be replayed.
    void chat_stream(json body, const std::function<bool(const json& chunk)>& on_chunk) const {
        body["stream"] = true;
        stream("/v1/chat/completions", body, on_chunk);
    }

    /// Convenience over chat_stream: hands you only the incremental text.
    /// Returns the accumulated answer.
    std::string chat_stream_text(json body, const std::function<void(std::string_view delta)>& on_delta,
                                 Usage* usage_out = nullptr) const {
        std::string full;
        chat_stream(std::move(body), [&](const json& chunk) {
            if (usage_out != nullptr && chunk.contains("usage") && !chunk["usage"].is_null()) {
                *usage_out = parse_usage(chunk);
            }
            const auto choices = chunk.find("choices");
            if (choices == chunk.end() || !choices->is_array() || choices->empty()) {
                return true;  // final usage-only chunk
            }
            const json& delta = (*choices)[0].value("delta", json::object());
            const auto content = delta.find("content");
            if (content != delta.end() && content->is_string()) {
                const std::string text = content->get<std::string>();
                full += text;
                if (on_delta) {
                    on_delta(text);
                }
            }
            return true;
        });
        return full;
    }

    /// POST /v1/responses (Responses API). Stateless: send the whole history each time.
    json responses(json body) const {
        return post("/v1/responses", body);
    }

    /// Streaming Responses API. Events carry a `type` field — see references/streaming.md.
    void responses_stream(json body, const std::function<bool(const json& event)>& on_event) const {
        body["stream"] = true;
        stream("/v1/responses", body, on_event);
    }

    // -- embeddings ---------------------------------------------------------

    json embeddings(std::string_view model, const std::vector<std::string>& input) const {
        return post("/v1/embeddings", json { { "model", model }, { "input", input } });
    }

    // -- media --------------------------------------------------------------

    /// POST /v1/media — returns a job object, usually with status "pending".
    json media_create(json body) const {
        return post("/v1/media", body);
    }

    /// GET /v1/media/{id}
    json media_status(std::string_view id) const {
        return get("/v1/media/" + std::string(id));
    }

    /// Polls until the job reaches a terminal state. Throws on `failed` and on timeout.
    json media_wait(std::string_view id, std::chrono::seconds interval = std::chrono::seconds(5),
                    std::chrono::seconds max_wait = std::chrono::seconds(600)) const {
        const auto deadline = std::chrono::steady_clock::now() + max_wait;
        while (true) {
            json status = media_status(id);
            const std::string state = status.value("status", "");
            if (state == "completed") {
                return status;
            }
            if (state == "failed" || state == "cancelled") {
                const json error = status.value("error", json::object());
                throw Error("media generation " + state + ": " + error.value("message", "unknown"), 0,
                            error.value("code", "MEDIA_FAILED"));
            }
            if (std::chrono::steady_clock::now() + interval > deadline) {
                throw Error("timed out waiting for media " + std::string(id), 0, "TIMEOUT");
            }
            std::this_thread::sleep_for(interval);
        }
    }

    /// Generate and wait in one call.
    json media_generate(json body) const {
        json job = media_create(std::move(body));
        if (job.value("status", "") == "completed") {
            return job;  // small images sometimes finish synchronously
        }
        return media_wait(job.at("id").get<std::string>());
    }

    // -- storage ------------------------------------------------------------

    /// POST /v1/storage/upload as multipart/form-data.
    /// `policy` is TEMP_UPLOAD (24 h, default) or PERMANENT.
    json upload_file(const std::string& file_path, std::string_view policy = "TEMP_UPLOAD",
                     std::string_view mime_type = "") const {
        CURL* curl = handle();
        std::string response;
        long status = 0;

        curl_mime* form = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(form);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, file_path.c_str());
        if (!mime_type.empty()) {
            curl_mime_type(part, std::string(mime_type).c_str());
        }
        part = curl_mime_addpart(form);
        curl_mime_name(part, "storagePolicy");
        curl_mime_data(part, std::string(policy).c_str(), CURL_ZERO_TERMINATED);

        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + _options.api_key).c_str());

        const std::string url = _options.base_url + "/v1/storage/upload";
        apply_common(curl, url, headers, &response);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        const CURLcode rc = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_mime_free(form);

        if (rc != CURLE_OK) {
            throw Error(std::string("upload failed: ") + curl_easy_strerror(rc), 0, "TRANSPORT");
        }
        return decode(response, status);
    }

    /// PATCH /v1/storage/files/{id}/keep — move a file to permanent storage.
    json keep_file(std::string_view file_id) const {
        return patch("/v1/storage/files/" + std::string(file_id) + "/keep");
    }

    // -- account ------------------------------------------------------------

    /// GET /v1/balance. The API returns the amount as a string.
    double balance() const {
        const json response = get("/v1/balance");
        const json amount = response.value("amount", json());
        if (amount.is_string()) {
            return std::stod(amount.get<std::string>());
        }
        return amount.is_number() ? amount.get<double>() : 0.0;
    }

    /// GET /v1/models, optionally filtered by type (chat, image, embedding, audio, video, tts, stt).
    json models(std::string_view type = "") const {
        std::string path = "/v1/models";
        if (!type.empty()) {
            path += "?type=" + std::string(type);
        }
        return get(path);
    }

private:
    // -- transport ----------------------------------------------------------

    static void global_init() {
        static const int once = [] {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            return 0;
        }();
        (void)once;
    }

    /// One easy handle per thread: curl_easy_reset clears the options but keeps
    /// the connection pool, so TLS is negotiated once per thread rather than
    /// once per request.
    static CURL* handle() {
        struct Deleter {
            void operator()(CURL* c) const {
                curl_easy_cleanup(c);
            }
        };
        thread_local std::unique_ptr<CURL, Deleter> curl { curl_easy_init() };
        if (!curl) {
            throw Error("curl_easy_init failed", 0, "TRANSPORT");
        }
        curl_easy_reset(curl.get());
        return curl.get();
    }

    static std::size_t write_to_string(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
        auto* out = static_cast<std::string*>(userdata);
        out->append(ptr, size * nmemb);
        return size * nmemb;
    }

    void apply_common(CURL* curl, const std::string& url, curl_slist* headers, std::string* sink) const {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Client::write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, sink);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, _options.timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _options.connect_timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, _options.user_agent.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        if (_options.verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }
    }

    curl_slist* json_headers() const {
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + _options.api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        return headers;
    }

    /// Turns a body+status pair into JSON, or into an Error carrying Polza's error.code.
    static json decode(const std::string& body, long status) {
        json parsed;
        bool ok = true;
        try {
            parsed = json::parse(body);
        } catch (const json::exception&) {
            ok = false;
        }
        if (status >= 200 && status < 300) {
            if (!ok) {
                throw Error("response is not valid JSON: " + body.substr(0, 200), status, "INVALID_JSON");
            }
            return parsed;
        }
        std::string code = "HTTP_" + std::to_string(status);
        std::string message = body.substr(0, 500);
        if (ok && parsed.contains("error")) {
            const json& error = parsed["error"];
            if (error.is_object()) {
                code = error.value("code", code);
                message = error.value("message", message);
            } else if (error.is_string()) {
                message = error.get<std::string>();
            }
        }
        throw Error("Polza API " + std::to_string(status) + ": " + message, status, code);
    }

    json request(std::string_view path, const json* body, const char* method) const {
        const std::string url = _options.base_url + std::string(path);
        const std::string payload = body != nullptr ? body->dump() : std::string();

        std::chrono::milliseconds backoff = _options.initial_backoff;
        for (int attempt = 0;; ++attempt) {
            CURL* curl = handle();
            curl_slist* headers = json_headers();
            std::string response;
            long status = 0;

            apply_common(curl, url, headers, &response);
            if (std::string_view(method) == "POST") {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
            } else if (std::string_view(method) != "GET") {
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
            }

            const CURLcode rc = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            curl_slist_free_all(headers);

            try {
                if (rc != CURLE_OK) {
                    throw Error(std::string("transport error: ") + curl_easy_strerror(rc), 0, "TRANSPORT");
                }
                return decode(response, status);
            } catch (const Error& error) {
                if (attempt >= _options.max_retries || !error.retryable()) {
                    throw;
                }
                std::this_thread::sleep_for(backoff);
                backoff *= 2;
            }
        }
    }

    struct StreamState {
        SseParser* parser;
        std::string* error_body;
        bool http_error;
        bool cancelled;
    };

    static std::size_t write_stream(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
        auto* state = static_cast<StreamState*>(userdata);
        const std::size_t bytes = size * nmemb;
        if (state->http_error) {
            state->error_body->append(ptr, bytes);  // non-200 bodies are plain JSON, not SSE
            return bytes;
        }
        if (!state->parser->feed(std::string_view(ptr, bytes))) {
            if (!state->parser->finished()) {
                return 0;
            }
            state->cancelled = true;
            return 0;  // aborts the transfer; reported as CURLE_WRITE_ERROR
        }
        return bytes;
    }

    void stream(std::string_view path, const json& body,
                const std::function<bool(const json&)>& on_event) const {
        const std::string url = _options.base_url + std::string(path);
        const std::string payload = body.dump();

        CURL* curl = handle();
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + _options.api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: text/event-stream");

        std::string error_body;
        SseParser parser([&](std::string_view chunk) {
            json event;
            try {
                event = json::parse(chunk);
            } catch (const json::exception&) {
                return true;  // ignore anything that is not JSON
            }
            return on_event(event);
        });
        StreamState state { &parser, &error_body, false, false };

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Client::write_stream);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, _options.timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _options.connect_timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, _options.user_agent.c_str());
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        // Header callback flips the state to "collect the body as an error" on non-200.
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &Client::on_header);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &state);
        if (_options.verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }

        const CURLcode rc = curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);

        if (state.http_error || status < 200 || status >= 300) {
            decode(error_body, status);  // always throws
        }
        if (rc != CURLE_OK && !(rc == CURLE_WRITE_ERROR && state.cancelled)) {
            throw Error(std::string("stream transport error: ") + curl_easy_strerror(rc), 0, "TRANSPORT");
        }
    }

    static std::size_t on_header(char* buffer, std::size_t size, std::size_t nitems, void* userdata) {
        auto* state = static_cast<StreamState*>(userdata);
        const std::string_view line(buffer, size * nitems);
        if (line.starts_with("HTTP/")) {
            const std::size_t space = line.find(' ');
            if (space != std::string_view::npos && space + 4 <= line.size()) {
                const int status = std::atoi(std::string(line.substr(space + 1, 3)).c_str());
                state->http_error = status < 200 || status >= 300;
            }
        }
        return size * nitems;
    }

    Options _options;
};

// ------------------------------------------------------------- utilities ---

/// Builds a `data:<mime>;base64,...` URI, the form every Polza endpoint accepts
/// for inline images, documents and audio.
std::string to_data_uri(std::string_view mime_type, std::string_view bytes);

/// Base64 without external dependencies (small enough not to warrant one).
inline std::string base64_encode(std::string_view input) {
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((input.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 2 < input.size(); i += 3) {
        const std::uint32_t triple = (static_cast<unsigned char>(input[i]) << 16)
                                   | (static_cast<unsigned char>(input[i + 1]) << 8)
                                   | static_cast<unsigned char>(input[i + 2]);
        out.push_back(alphabet[(triple >> 18) & 0x3F]);
        out.push_back(alphabet[(triple >> 12) & 0x3F]);
        out.push_back(alphabet[(triple >> 6) & 0x3F]);
        out.push_back(alphabet[triple & 0x3F]);
    }
    if (i < input.size()) {
        std::uint32_t triple = static_cast<unsigned char>(input[i]) << 16;
        const bool has_second = i + 1 < input.size();
        if (has_second) {
            triple |= static_cast<unsigned char>(input[i + 1]) << 8;
        }
        out.push_back(alphabet[(triple >> 18) & 0x3F]);
        out.push_back(alphabet[(triple >> 12) & 0x3F]);
        out.push_back(has_second ? alphabet[(triple >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

inline std::string base64_decode(std::string_view input) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::string out;
    int buffer = 0;
    int bits = 0;
    for (const char c : input) {
        const int decoded = value(c);
        if (decoded < 0) {
            continue;  // skips '=', newlines and any data-URI prefix leftovers
        }
        buffer = (buffer << 6) | decoded;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buffer >> bits) & 0xFF));
        }
    }
    return out;
}

inline std::string to_data_uri(std::string_view mime_type, std::string_view bytes) {
    return "data:" + std::string(mime_type) + ";base64," + base64_encode(bytes);
}

/// Reads a whole file into a string; throws Error so callers have one catch site.
inline std::string read_file(const std::string& path);

}  // namespace polza

#include <fstream>
#include <sstream>

namespace polza {

inline std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw Error("cannot open file: " + path, 0, "IO");
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}  // namespace polza