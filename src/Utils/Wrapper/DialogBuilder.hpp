#pragma once
#include <string>
#include <fmt/core.h>

enum eSizeType : uint8_t {
    SMALL,
    BIG
};
enum eDirection : uint8_t {
    NONE,
    LEFT,
    RIGHT,
    STATIC_BLUE_FRAME,

    DIR_BIG
};

class DialogBuilder {
public:
    DialogBuilder();
    ~DialogBuilder();

    DialogBuilder* SetDefaultColor(char color);
    DialogBuilder* TextScalingString(std::string scale);

    DialogBuilder* EmbedData(std::string name, std::string value);
    template<typename T, typename std::enable_if_t<std::is_integral_v<T>, bool> = true>
    DialogBuilder* EmbedData(std::string name, T value);
    template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
    DialogBuilder* EmbedData(std::string name, T value);

    DialogBuilder* AddCustomBreak();
    DialogBuilder* AddSpacer(eSizeType size = SMALL);
    DialogBuilder* SetCustomSpacing(int32_t x, int32_t y);
    DialogBuilder* AddLabel(std::string label, eDirection direction =  LEFT, eSizeType size = SMALL);
    DialogBuilder* AddLabelWithIcon(std::string label, int32_t itemId, eDirection direction =  LEFT, eSizeType size = SMALL);
    DialogBuilder* AddLabelWithEleIcon(std::string label, int32_t itemId, eDirection direction = LEFT, eSizeType size = SMALL);
    DialogBuilder* AddCustomLabel(std::string label, std::string target, double top, double left, eSizeType size = SMALL);
    DialogBuilder* AddTextbox(std::string label);
    DialogBuilder* AddSmallText(std::string label);
    DialogBuilder* AddTextInput(std::string name, std::string label, std::string labelInside, int32_t maxLength);
    DialogBuilder* AddTextInputPassword(std::string name, std::string label, std::string labelInside, int32_t maxLength);
    DialogBuilder* AddTextboxInput(std::string name, std::string label, std::string textInside, int32_t maxLength, int32_t lines);
    DialogBuilder* AddButton(std::string name, std::string label, std::string buttonFlag = "noflags");

    DialogBuilder* EndDialog(std::string name, std::string cancel, std::string ok);

public:
    std::string Get() const { return m_result; }
    operator std::string() { return m_result; }

private:
    std::string GetSize(uint8_t size) const;
    std::string GetDirection(uint8_t direction) const;

private:
    std::string m_result;
};
