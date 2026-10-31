#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

namespace Windget {

enum class JsonType
{
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue
{
public:
    JsonValue();
    JsonValue(bool value);
    JsonValue(int value);
    JsonValue(double value);
    JsonValue(const char* value);
    JsonValue(std::string value);
    JsonValue(std::vector<JsonValue> array);

    bool IsNull() const { return m_type == JsonType::Null; }
    bool IsBool() const { return m_type == JsonType::Boolean; }
    bool IsNumber() const { return m_type == JsonType::Number; }
    bool IsString() const { return m_type == JsonType::String; }
    bool IsArray() const { return m_type == JsonType::Array; }
    bool IsObject() const { return m_type == JsonType::Object; }

    JsonType GetType() const { return m_type; }

    bool AsBool(bool defaultValue = false) const;
    double AsDouble(double defaultValue = 0.0) const;
    int AsInt(int defaultValue = 0) const;
    std::string AsString(const std::string& defaultValue = "") const;

    const std::vector<JsonValue>& AsArray() const;
    const std::vector<std::pair<std::string, JsonValue>>& AsObject() const;

    bool Contains(const std::string& key) const;
    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t index) const;

    void Set(const std::string& key, JsonValue value);
    void PushBack(JsonValue value);

    // 辅助构造空对象/数组
    static JsonValue MakeObject();
    static JsonValue MakeArray();

    // 序列化为格式化 JSON 字符串
    std::string Serialize(int indent = 4) const;

    // 解析 JSON 文本
    static JsonValue Parse(const std::string& jsonText, std::string* errorOut = nullptr);

    static const JsonValue NullVal;

private:
    std::string SerializeInternal(int currentIndent, int indentStep) const;

private:
    JsonType m_type = JsonType::Null;
    bool m_boolValue = false;
    double m_numberValue = 0.0;
    std::string m_stringValue;
    std::vector<JsonValue> m_arrayValue;
    std::vector<std::pair<std::string, JsonValue>> m_objectValue;
};

} // namespace Windget
