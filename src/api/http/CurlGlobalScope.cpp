#include "CurlGlobalScope.hpp"
#include <curl/curl.h>
#include <stdexcept>

namespace
{
void AssertIsGlobalInitSuccessful(const CURLcode result)
{
	if (result != CURLE_OK)
	{
		throw std::runtime_error("Не удалось инициализировать HTTP-подсистему");
	}
}
} // namespace

CurlGlobalScope::CurlGlobalScope()
{
	AssertIsGlobalInitSuccessful(curl_global_init(CURL_GLOBAL_DEFAULT));
}

CurlGlobalScope::~CurlGlobalScope()
{
	curl_global_cleanup();
}