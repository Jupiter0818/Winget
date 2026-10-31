#include "utils/JsonUtils.h"
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstdint>

namespace Windget {

const JsonValue JsonValue::NullVal{};

JsonValue::JsonValue()
    : m_type(JsonType::Null)
{
}

JsonValue::JsonValue(bool value)
    : m_type(JsonType::Boolean), m_boolValue(value)
{
}

JsonValue::JsonValue(int value)
    : m_type(JsonType::Number), m_numberValue(static_cast<double>(value))
{
}

JsonValue::JsonValue(double value)
    : m_type(JsonType::Number), m_numberValue(value)
{
}

JsonValue::JsonValue(const char* value)
    : m_type(JsonType::String), m_stringValue(value ? value : "")
{
}

JsonValue::JsonValue(std::string value)
    : m_type(JsonType::String), m_stringValue(std::move(value))
{
}

JsonValue::JsonValue(std::vector<JsonValue> array)
    : m_type(JsonType::Array), m_arrayValue(std::move(array))
{
}

JsonValue JsonValue::MakeObject()
{
    JsonValue val;
    val.m_type = JsonType::Object;
    return val;
}

JsonValue JsonValue::MakeArray()
{
    JsonValue val;
    val.m_type = JsonType::Array;
    return val;
}

bool JsonValue::AsBool(bool defaultValue) const
{
    if (m_type == JsonType::Boolean) return m_boolValue;
    if (m_type == JsonType::Number) return m_numberValue != 0.0;
    return defaultValue;
}

double JsonValue::AsDouble(double defaultValue) const
{
    if (m_type == JsonType::Number) return m_numberValue;
    return defaultValue;
}

int JsonValue::AsInt(int defaultValue) const
{
    if (m_type == JsonType::Number) return static_cast<int>(m_numberValue);
    return defaultValue;
}

std::string JsonValue::AsString(const std::string& defaultValue) const
{
    if (m_type == JsonType::String) return m_stringValue;
    return defaultValue;
}

const std::vector<JsonValue>& JsonValue::AsArray() const
{
    static const std::vector<JsonValue> emptyArr;
    return (m_type == JsonType::Array) ? m_arrayValue : emptyArr;
}

const std::vector<std::pair<std::string, JsonValue>>& JsonValue::AsObject() const
{
    static const std::vector<std::pair<std::string, JsonValue>> emptyObj;
    return (m_type == JsonType::Object) ? m_objectValue : emptyObj;
}

bool JsonValue::Contains(const std::string& key) const
{
    if (m_type != JsonType::Object) return false;
    for (const auto& pair : m_objectValue)
    {
        if (pair.first == key) return true;
    }
    return false;
}

const JsonValue& JsonValue::operator[](const std::string& key) const
{
    if (m_type == JsonType::Object)
    {
        for (const auto& pair : m_objectValue)
        {
            if (pair.first == key) return pair.second;
        }
    }
    return NullVal;
}

const JsonValue& JsonValue::operator[](size_t index) const
{
    if (m_type == JsonType::Array && index < m_arrayValue.size())
    {
        return m_arrayValue[index];
    }
    return NullVal;
}

void JsonValue::Set(const std::string& key, JsonValue value)
{
    m_type = JsonType::Object;
    for (auto& pair : m_objectValue)
    {
        if (pair.first == key)
        {
            pair.second = std::move(value);
            return;
        }
    }
    m_objectValue.emplace_back(key, std::move(value));
}

void JsonValue::PushBack(JsonValue value)
{
    m_type = JsonType::Array;
    m_arrayValue.push_back(std::move(value));
}

std::string JsonValue::Serialize(int indent) const
{
    return SerializeInternal(0, indent);
}

std::string JsonValue::SerializeInternal(int currentIndent, int indentStep) const
{
    std::string indentStr(currentIndent, ' ');
    std::string nextIndentStr(currentIndent + indentStep, ' ');

    switch (m_type)
    {
    case JsonType::Null:
        return "null";

    case JsonType::Boolean:
        return m_boolValue ? "true" : "false";

    case JsonType::Number:
    {
        // 如果是整数则不输出小数点
        if (m_numberValue == static_cast<double>(static_cast<int64_t>(m_numberValue)))
        {
            return std::to_string(static_cast<int64_t>(m_numberValue));
        }
        std::string s = std::to_string(m_numberValue);
        // 去除多余的 0
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }

    case JsonType::String:
    {
        std::string escaped = "\"";
        for (char c : m_stringValue)
        {
            if (c == '\"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }

    case JsonType::Array:
    {
        if (m_arrayValue.empty()) return "[]";
        std::string out = "[\n";
        for (size_t i = 0; i < m_arrayValue.size(); ++i)
        {
            out += nextIndentStr + m_arrayValue[i].SerializeInternal(currentIndent + indentStep, indentStep);
            if (i + 1 < m_arrayValue.size()) out += ",";
            out += "\n";
        }
        out += indentStr + "]";
        return out;
    }

    case JsonType::Object:
    {
        if (m_objectValue.empty()) return "{}";
        std::string out = "{\n";
        for (size_t i = 0; i < m_objectValue.size(); ++i)
        {
            out += nextIndentStr + "\"" + m_objectValue[i].first + "\": ";
            out += m_objectValue[i].second.SerializeInternal(currentIndent + indentStep, indentStep);
            if (i + 1 < m_objectValue.size()) out += ",";
            out += "\n";
        }
        out += indentStr + "}";
        return out;
    }
    }

    return "null";
}

namespace {

class JsonParser
{
public:
    explicit JsonParser(const std::string& text)
        : m_text(text), m_index(0)
    {
    }

    JsonValue Parse(std::string* errorOut)
    {
        SkipWhitespace();
        if (m_index >= m_text.size())
        {
            if (errorOut) *errorOut = "Empty text";
            return JsonValue::NullVal;
        }

        JsonValue val = ParseValue();
        SkipWhitespace();
        return val;
    }

private:
    void SkipWhitespace()
    {
        while (m_index < m_text.size() && (std::isspace(static_cast<unsigned char>(m_text[m_index]))))
        {
            m_index++;
        }
    }

    char Peek() const
    {
        return (m_index < m_text.size()) ? m_text[m_index] : '\0';
    }

    char Get()
    {
        return (m_index < m_text.size()) ? m_text[m_index++] : '\0';
    }

    JsonValue ParseValue()
    {
        SkipWhitespace();
        char c = Peek();

        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '\"') return ParseString();
        if (c == 't' || c == 'f') return ParseBool();
        if (c == 'n') return ParseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber();

        return JsonValue::NullVal;
    }

    JsonValue ParseObject()
    {
        Get(); // consume '{'
        JsonValue obj = JsonValue::MakeObject();

        SkipWhitespace();
        if (Peek() == '}')
        {
            Get();
            return obj;
        }

        while (m_index < m_text.size())
        {
            SkipWhitespace();
            if (Peek() != '\"') break;

            JsonValue keyVal = ParseString();
            std::string key = keyVal.AsString();

            SkipWhitespace();
            if (Peek() == ':') Get();
            else break;

            JsonValue val = ParseValue();
            obj.Set(key, std::move(val));

            SkipWhitespace();
            char next = Peek();
            if (next == ',')
            {
                Get();
            }
            else if (next == '}')
            {
                Get();
                break;
            }
            else
            {
                break;
            }
        }

        return obj;
    }

    JsonValue ParseArray()
    {
        Get(); // consume '['
        JsonValue arr = JsonValue::MakeArray();

        SkipWhitespace();
        if (Peek() == ']')
        {
            Get();
            return arr;
        }

        while (m_index < m_text.size())
        {
            JsonValue val = ParseValue();
            arr.PushBack(std::move(val));

            SkipWhitespace();
            char next = Peek();
            if (next == ',')
            {
                Get();
            }
            else if (next == ']')
            {
                Get();
                break;
            }
            else
            {
                break;
            }
        }

        return arr;
    }

    JsonValue ParseString()
    {
        Get(); // consume '\"'
        std::string str;

        while (m_index < m_text.size())
        {
            char c = Get();
            if (c == '\"')
            {
                break;
            }
            if (c == '\\')
            {
                char esc = Get();
                if (esc == '\"') str += '\"';
                else if (esc == '\\') str += '\\';
                else if (esc == '/') str += '/';
                else if (esc == 'n') str += '\n';
                else if (esc == 'r') str += '\r';
                else if (esc == 't') str += '\t';
                else str += esc;
            }
            else
            {
                str += c;
            }
        }

        return JsonValue(std::move(str));
    }

    JsonValue ParseNumber()
    {
        size_t start = m_index;
        if (Peek() == '-') Get();

        while (m_index < m_text.size() && std::isdigit(static_cast<unsigned char>(Peek())))
        {
            Get();
        }

        if (Peek() == '.')
        {
            Get();
            while (m_index < m_text.size() && std::isdigit(static_cast<unsigned char>(Peek())))
            {
                Get();
            }
        }

        std::string numStr = m_text.substr(start, m_index - start);
        try
        {
            double val = std::stod(numStr);
            return JsonValue(val);
        }
        catch (...)
        {
            return JsonValue(0.0);
        }
    }

    JsonValue ParseBool()
    {
        if (m_text.compare(m_index, 4, "true") == 0)
        {
            m_index += 4;
            return JsonValue(true);
        }
        if (m_text.compare(m_index, 5, "false") == 0)
        {
            m_index += 5;
            return JsonValue(false);
        }
        return JsonValue::NullVal;
    }

    JsonValue ParseNull()
    {
        if (m_text.compare(m_index, 4, "null") == 0)
        {
            m_index += 4;
        }
        return JsonValue::NullVal;
    }

private:
    const std::string& m_text;
    size_t m_index;
};

} // namespace

JsonValue JsonValue::Parse(const std::string& jsonText, std::string* errorOut)
{
    JsonParser parser(jsonText);
    return parser.Parse(errorOut);
}

} // namespace Windget
