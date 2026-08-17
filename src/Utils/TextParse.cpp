#include <Utils/TextParse.hpp>
#include <algorithm>
#include <type_traits>

TextParse::TextParse() : m_data() {}
TextParse::TextParse(const std::string& string) {
    this->Parse(string);
}
TextParse::TextParse(const std::vector<std::pair<std::string, std::string>>& data) {
    for (auto it : data)
        this->Add(it.first, it.second);
}
TextParse::~TextParse() { m_data.clear(); }

std::vector<std::string> TextParse::StringTokenize(const std::string& string, const std::string& delimiter) {
    std::vector<std::string> tokens{};
    size_t previousPos = 0, currentPos = 0;
    do {
        currentPos = string.find(delimiter, previousPos);
        if (currentPos == std::string::npos) currentPos = string.length();
        std::string token = string.substr(previousPos, currentPos - previousPos);
        if (!token.empty()) tokens.push_back(token);
        previousPos = currentPos + delimiter.length();
    } while (currentPos < string.length() && previousPos < string.length());
    return tokens;
}

void TextParse::Parse(const std::string& string) {

    std::string cleanString = string;
    cleanString.erase(std::remove(cleanString.begin(), cleanString.end(), '\r'), cleanString.end());

    m_data = TextParse::StringTokenize(cleanString, "\n");
    for (size_t i = 0; i < m_data.size(); i++) {
        auto& iterator = m_data[i];

        size_t ridPos = iterator.find("rid|");
        if (ridPos != std::string::npos) {
            size_t ridValStart = ridPos + 4;
            size_t platPos = iterator.find("platformID|", ridValStart);
            if (platPos != std::string::npos && platPos > ridValStart) {
                std::string secondPart = iterator.substr(platPos);
                iterator.erase(platPos);
                m_data.insert(m_data.begin() + i + 1, secondPart);
            }
        }
    }
}

std::string TextParse::Get(const std::string& key, int index, const std::string& token, int key_index) {
    if (m_data.empty())
        return std::string{ "" };

    for (auto& iterator : m_data) {
        if (iterator.empty())
            continue;

        std::vector<std::string> tokenize = TextParse::StringTokenize(iterator, token);
        if (tokenize.size() <= key_index || tokenize[key_index] != key)
            continue;
        if (index < 0 || index >= tokenize.size())
            return std::string{ "" };
        return tokenize[key_index + index];
    }
    return std::string{ "" };
}

template <typename T, typename std::enable_if_t<std::is_floating_point_v<T>, bool>>
T TextParse::Get(const std::string& key, int index, const std::string& token) {
    if (std::is_same_v<T, double>)
        return std::stod(this->Get(key, index, token));
    else if (std::is_same_v<T, long double>)
        return std::stold(this->Get(key, index, token));
    return std::stof(this->Get(key, index, token));
}

void TextParse::Set(const std::string& key, const std::string& value, const std::string& token) {
    if (m_data.empty())
        return;
    for (auto& iterator : m_data) {
        std::vector<std::string> tokenize = TextParse::StringTokenize(iterator, token);
        if (tokenize.empty() || tokenize[0] != key)
            continue;
        iterator = std::string{ tokenize[0] };
        iterator += token;
        iterator += value;
        break;
    }
}

bool TextParse::IsEmpty() const {
    return m_data.empty();
}

size_t TextParse::GetSize() const {
    return m_data.size();
}
std::string TextParse::GetAsString() const {
    std::string ret{};
    for (auto iterator : m_data)
        ret.append(iterator + "\n");
    return ret;
}
