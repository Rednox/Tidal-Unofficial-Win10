#include "pch.h"
#include "GetListSublistsQuery.h"
#include "../tools/StringUtils.h"

std::wstring api::GetListSublistsQuery::url() const
{
	std::wstring result(_list->Data());
	result.push_back(L'/');
	return result;
}

concurrency::task<std::shared_ptr<std::vector<api::SublistInfo>>> api::GetListSublistsQuery::executeAsync(concurrency::cancellation_token cancelToken)
{
	return getAsync(cancelToken).then([](Platform::String^ json) {
		tools::strings::WindowsWIStream stream(json);
		auto jsonVal = web::json::value::parse(stream);
		auto result = std::make_shared<std::vector<api::SublistInfo>>();
		for (auto& val : jsonVal.as_array()) {
			result->emplace_back(val);
		}
		return result;
	});
}
