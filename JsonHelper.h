// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <string>

namespace winrt
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Data::Json;
}

struct JsonValueMaker : winrt::Windows::Data::Json::IJsonValueStatics
{
    JsonValueMaker() : IJsonValueStatics(winrt::get_activation_factory<winrt::Windows::Data::Json::JsonValue, IJsonValueStatics>()) {}
};

inline std::wstring guid_to_wstring(const winrt::guid& value)
{
    auto converted = winrt::to_hstring(value);
    auto stringView = std::wstring_view(converted);
    std::wstring fullString;
    if (stringView.size() == 38 && ((stringView[0] == L'{' && stringView[37] == L'}') || (stringView[0] == L'(' && stringView[37] == L')')))
    {
        fullString.assign(stringView.data() + 1, stringView.size() - 2);
    }
    else
    {
        fullString = stringView;
    }
    return fullString;
}

template<typename T>
winrt::IJsonValue NumberArrayToJson(winrt::com_array<T> const& numberArray)
{
    winrt::JsonArray jsonArray;
    JsonValueMaker valueMaker;
    for (const auto& number : numberArray)
    {
        jsonArray.Append(valueMaker.CreateStringValue(std::to_wstring(number)));
    }

    return jsonArray;
}

winrt::IJsonValue RectToJsonArray(winrt::Windows::Foundation::Rect const& rect)
{
    winrt::JsonArray jsonArray;
    JsonValueMaker valueMaker;

    jsonArray.ReplaceAll({
        valueMaker.CreateStringValue(std::to_wstring(rect.X)),
        valueMaker.CreateStringValue(std::to_wstring(rect.Y)),
        valueMaker.CreateStringValue(std::to_wstring(rect.Height)),
        valueMaker.CreateStringValue(std::to_wstring(rect.Width)) });

    return jsonArray;
}

winrt::IJsonValue DateTimeToJson(winrt::Windows::Foundation::DateTime const& dateTime)
{
    JsonValueMaker valueMaker;
    auto sysTime = winrt::clock::to_sys(dateTime);
    int64_t timeIntegerRep = std::chrono::duration_cast<std::chrono::milliseconds>(sysTime.time_since_epoch()).count();
    return valueMaker.CreateStringValue(std::to_wstring(timeIntegerRep));
}

winrt::IJsonValue TimeSpanToJson(winrt::Windows::Foundation::TimeSpan const& timeSpan)
{
    JsonValueMaker valueMaker;
    int64_t durationIntegerRep = std::chrono::duration_cast<std::chrono::milliseconds>(timeSpan).count();
    return valueMaker.CreateStringValue(std::to_wstring(durationIntegerRep));
}

winrt::IJsonValue TypedPropertyValueToJson(winrt::IPropertyValue const& propertyValue)
{
    JsonValueMaker valueMaker;
    winrt::IJsonValue jsonValue;
    switch (propertyValue.Type())
    {
    case winrt::PropertyType::Int16:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetInt16()));
        break;
    case winrt::PropertyType::Int32:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetInt32()));
        break;
    case winrt::PropertyType::UInt8:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetUInt8()));
        break;
    case winrt::PropertyType::UInt16:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetUInt16()));
        break;
    case winrt::PropertyType::UInt32:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetUInt32()));
        break;
    case winrt::PropertyType::Single:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetSingle()));
        break;
    case winrt::PropertyType::Double:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetDouble()));
        break;
    case winrt::PropertyType::Char16:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetChar16()));
        break;
    case winrt::PropertyType::Int64:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetInt64()));
        break;
    case winrt::PropertyType::UInt64:
        jsonValue = valueMaker.CreateStringValue(std::to_wstring(propertyValue.GetUInt64()));
        break;
    case winrt::PropertyType::Guid:
        jsonValue = valueMaker.CreateStringValue(guid_to_wstring(propertyValue.GetGuid()));
        break;
    case winrt::PropertyType::Rect:
        jsonValue = RectToJsonArray(propertyValue.GetRect());
        break;
    case winrt::PropertyType::Int16Array:
    {
        winrt::com_array<int16_t> numberArray;
        propertyValue.GetInt16Array(numberArray);
        jsonValue = NumberArrayToJson<int16_t>(numberArray);
        break;
    }
    case winrt::PropertyType::Int32Array:
    {
        winrt::com_array<int32_t> numberArray;
        propertyValue.GetInt32Array(numberArray);
        jsonValue = NumberArrayToJson<int32_t>(numberArray);
        break;
    }
    case winrt::PropertyType::UInt8Array:
    {
        winrt::com_array<uint8_t> numberArray;
        propertyValue.GetUInt8Array(numberArray);
        jsonValue = NumberArrayToJson<uint8_t>(numberArray);
        break;
    }
    case winrt::PropertyType::UInt16Array:
    {
        winrt::com_array<uint16_t> numberArray;
        propertyValue.GetUInt16Array(numberArray);
        jsonValue = NumberArrayToJson<uint16_t>(numberArray);
        break;
    }
    case winrt::PropertyType::UInt32Array:
    {
        winrt::com_array<uint32_t> numberArray;
        propertyValue.GetUInt32Array(numberArray);
        jsonValue = NumberArrayToJson<uint32_t>(numberArray);
        break;
    }
    case winrt::PropertyType::SingleArray:
    {
        winrt::com_array<float_t> numberArray;
        propertyValue.GetSingleArray(numberArray);
        jsonValue = NumberArrayToJson<float_t>(numberArray);
        break;
    }
    case winrt::PropertyType::DoubleArray:
    {
        winrt::com_array<double_t> numberArray;
        propertyValue.GetDoubleArray(numberArray);
        jsonValue = NumberArrayToJson<double_t>(numberArray);
        break;
    }
    case winrt::PropertyType::Char16Array:
    {
        winrt::com_array<char16_t> numberArray;
        propertyValue.GetChar16Array(numberArray);
        jsonValue = NumberArrayToJson<char16_t>(numberArray);
        break;
    }
    case winrt::PropertyType::Int64Array:
    {
        winrt::com_array<int64_t> numberArray;
        propertyValue.GetInt64Array(numberArray);
        jsonValue = NumberArrayToJson<int64_t>(numberArray);
        break;
    }
    case winrt::PropertyType::UInt64Array:
    {
        winrt::com_array<uint64_t> numberArray;
        propertyValue.GetUInt64Array(numberArray);
        jsonValue = NumberArrayToJson<uint64_t>(numberArray);
        break;
    }
    case winrt::PropertyType::DateTime:
    {
        jsonValue = DateTimeToJson(propertyValue.GetDateTime());
        break;
    }
    case winrt::PropertyType::TimeSpan:
    {
        jsonValue = TimeSpanToJson(propertyValue.GetTimeSpan());
        break;
    }
    default:
        throw winrt::hresult_invalid_argument();
    }

    return jsonValue;
}

winrt::JsonObject SerializeValueSet(winrt::ValueSet const& valueSet)
{
    winrt::JsonObject jsonObject;
    for (auto&& pair : valueSet)
    {
        winrt::hstring key = pair.Key();
        if (auto nestedValueSet = pair.Value().try_as<winrt::ValueSet>())
        {
            jsonObject.Insert(key, SerializeValueSet(nestedValueSet));
            continue;
        }

        winrt::IPropertyValue propertyValue = pair.Value().as<winrt::IPropertyValue>();
        switch (propertyValue.Type())
        {
        case winrt::PropertyType::String:
        {
            const winrt::hstring value = propertyValue.GetString();
            jsonObject.Insert(key, winrt::JsonValue::CreateStringValue(value));
            break;
        }
        case winrt::PropertyType::Boolean:
        {
            const bool value = propertyValue.GetBoolean();
            jsonObject.Insert(key, winrt::JsonValue::CreateBooleanValue(value));
            break;
        }
        case winrt::PropertyType::Int16:
        case winrt::PropertyType::Int32:
        case winrt::PropertyType::UInt8:
        case winrt::PropertyType::UInt16:
        case winrt::PropertyType::UInt32:
        case winrt::PropertyType::Single:
        case winrt::PropertyType::Double:
        case winrt::PropertyType::Char16:
        case winrt::PropertyType::Int64:
        case winrt::PropertyType::UInt64:
        case winrt::PropertyType::Guid:
        case winrt::PropertyType::Int16Array:
        case winrt::PropertyType::Int32Array:
        case winrt::PropertyType::UInt8Array:
        case winrt::PropertyType::UInt16Array:
        case winrt::PropertyType::UInt32Array:
        case winrt::PropertyType::SingleArray:
        case winrt::PropertyType::DoubleArray:
        case winrt::PropertyType::Char16Array:
        case winrt::PropertyType::Int64Array:
        case winrt::PropertyType::UInt64Array:
        case winrt::PropertyType::Rect:
        case winrt::PropertyType::DateTime:
        case winrt::PropertyType::TimeSpan:
        {
            jsonObject.Insert(key, TypedPropertyValueToJson(propertyValue));
            break;
        }
        case winrt::PropertyType::StringArray:
        {
            winrt::com_array<winrt::hstring> stringArray;
            propertyValue.GetStringArray(stringArray);

            winrt::JsonArray jsonArray;
            for (const auto& value : stringArray)
            {
                jsonArray.Append(winrt::JsonValue::CreateStringValue(value));
            }

            jsonObject.Insert(key, jsonArray);
            break;
        }
        case winrt::PropertyType::BooleanArray:
        {
            winrt::com_array<bool> boolArray;
            propertyValue.GetBooleanArray(boolArray);

            winrt::JsonArray jsonArray;
            for (const auto& value : boolArray)
            {
                jsonArray.Append(winrt::JsonValue::CreateBooleanValue(value));
            }

            jsonObject.Insert(key, jsonArray);
            break;
        }
        default:
            // Add support for other types as needed.
            throw winrt::hresult_not_implemented();
        }
    }

    return jsonObject;
}