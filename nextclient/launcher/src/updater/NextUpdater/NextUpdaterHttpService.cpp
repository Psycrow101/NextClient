#include "NextUpdaterHttpService.h"
#include <cpr/api.h>
#include <tao/json.hpp>
#include <utility>
#include <data_encoding/aes.h>
#include <data_encoding/base64.h>
#include <nitro_utils/random_utils.h>
#include <next_launcher/version.h>
#include "NextUpdaterHttpServiceConfig.h"

using namespace concurrencpp;
using namespace std::chrono_literals;

NextUpdaterHttpService::NextUpdaterHttpService(std::string service_url, int connect_timeout_ms, std::shared_ptr<next_launcher::UserInfoClient> user_info) :
    service_url_(std::move(service_url)),
    connect_timeout_ms_(connect_timeout_ms),
    user_info_(user_info)
{
    headers_["Content-type"] = "application/json";
    headers_["UID"] = user_info_->GetClientUid();
    headers_["Branch"] = user_info_->GetUpdateBranch();
    headers_["LaunchGameCount"] = std::to_string(user_info_->GetLaunchGameCount());
    headers_["BuildVersion"] = NEXT_CLIENT_BUILD_VERSION;

    thread_executor_ = cc_runtime_.make_executor<thread_pool_executor>("HttpService thread pool", 2, 1min);
    update_executor_ = cc_runtime_.make_manual_executor();
}

HttpResponse NextUpdaterHttpService::Post(const std::string &method, const std::string& data)
{
    return PostInternal(method, data);
}

std::shared_ptr<CancelationToken> NextUpdaterHttpService::PostAsync(const std::string &method, const std::string& data, const std::function<void(const HttpResponse&)>& callback)
{
    auto cancelation_token = std::make_shared<CancelationToken>();
    auto data_reader = [data](){ return data; };

    thread_executor_->post([this, method, data_reader, cancelation_token, cb = callback]() { return PostCoroutine(method, data_reader, cancelation_token, cb); });

    return cancelation_token;
}

void NextUpdaterHttpService::Update()
{
    update_executor_->loop_once();
}

result<void> NextUpdaterHttpService::PostCoroutine(std::string method, std::function<std::string()> data_reader_threaded, std::shared_ptr<CancelationToken> cancelation_token, std::function<void(const HttpResponse&)> callback)
{
    HttpResponse result = PostInternal(method, data_reader_threaded());

    co_await resume_on(update_executor_);
    if (cancelation_token->IsCanceled())
        co_return;

    if (callback)
        callback(result);
}

HttpResponse NextUpdaterHttpService::PostInternal(const std::string &method, const std::string& data)
{
    std::string str = data;
    str.insert(0, "{\"method\": \"" + method + "\", \"data\": ");
    str.append("}");

    cpr::Session session;
    session.SetUrl(cpr::Url(service_url_));
    session.SetBody(SerializeAes(str));
    headers_["Client-Time"] = std::to_string(std::time(nullptr));
    session.SetHeader(headers_);
    session.SetConnectTimeout(connect_timeout_ms_);

    auto result = session.Post();

    if (result.status_code != 200 || result.error)
        return HttpResponse(result.status_code, result.error, "");

    if (result.text.empty() || result.text == "OK")
        return HttpResponse(result.status_code, result.error, result.text);

    try
    {
        std::string response = DeserializeAes(result.text);
        return HttpResponse(result.status_code, result.error, response);
    }
    catch (tao::json::pegtl::parse_error& e)
    { }

    cpr::Error error;
    error.code = cpr::ErrorCode::UNKNOWN_ERROR;
    error.message = "deserialization error";
    return HttpResponse(result.status_code, error, "");
}

std::string NextUpdaterHttpService::SerializeAes(const std::string &json_str)
{
    size_t data_len = json_str.size() + 1; // \0 at end
    size_t cipher_length = ((data_len / 16) + ((data_len % 16) > 0 ? 1 : 0)) * 16; // 16 for 128bit AES

    auto input_data = std::make_unique<uint8_t[]>(cipher_length);
    SecureZeroMemory(input_data.get(), cipher_length);

    auto output_data = std::make_unique<uint8_t[]>(cipher_length);
    SecureZeroMemory(output_data.get(), cipher_length);

    strcpy_s(reinterpret_cast<char*>(input_data.get()), data_len, json_str.c_str());

    auto iv = nitro_utils::GenerateRandomBytesSecure(16); // for aes
    AES128_CBC_encrypt_buffer(output_data.get(), input_data.get(), cipher_length, kHttpServiceCipherKey, iv.data());

    auto data_base64 = base64_encode(output_data.get(), cipher_length);
    auto iv_base64 = base64_encode(iv.data(), 16);

    tao::json::value v = {
        { "iv", iv_base64 },
        { "data", data_base64 }
    };

    return tao::json::to_string(v);
}

std::string NextUpdaterHttpService::DeserializeAes(const std::string &json_str)
{
    static_assert(sizeof(uint8_t) == sizeof(char), "uint8_t has different size than char");

    tao::json::value v;
    try
    {
        v = tao::json::from_string(json_str);
    }
    catch (...)
    {
        return "";
    }

    auto data_base64 = v.optional<std::string>("data");
    auto iv_base64 = v.optional<std::string>("iv");

    if(!data_base64 || !iv_base64)
        return "";

    auto data = base64_decode(*data_base64);
    auto iv = base64_decode(*iv_base64);

    if (iv.size() != 16)
        return "";

    if (data.size() < 16)
        return "";

    std::vector<uint8_t> decrypted_data(data.size());
    ZeroMemory(decrypted_data.data(), decrypted_data.size());
    AES128_CBC_decrypt_buffer(decrypted_data.data(), (uint8_t*)data.data(), data.size(), kHttpServiceCipherKey, (uint8_t*)iv.data());
    decrypted_data.push_back('\0');

    return reinterpret_cast<const char *>(decrypted_data.data());
}
