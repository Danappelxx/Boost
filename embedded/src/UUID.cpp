#include "UUID.h"

// 16 bit
UUID::UUID(uint16_t data) {
    this->data = { LOW_BYTE(data), HIGH_BYTE(data) };
}

// 128 bit
UUID::UUID(const uint8_t (&data)[16]) {
    this->data = std::vector<uint8_t>(data, data + 16);
}

// converts a single hex char to a number (0 - 15)
// https://github.com/graeme-hill/crossguid/blob/master/src/guid.cpp#L161
uint8_t hexDigitToChar(char ch) {
    // 0-9
    if (ch > 47 && ch < 58)
        return ch - 48;

    // a-f
    if (ch > 96 && ch < 103)
        return ch - 87;

    // A-F
    if (ch > 64 && ch < 71)
        return ch - 55;

    return 0;
}

// 128 bit
UUID::UUID(const std::string string) {
    std::vector<uint8_t> data;

    // char 1 is upper bytes, char 2 is lower bytes
    char char1;
    char char2;
    bool wantsCharOne = true;
    for (char c : string) {
        if (c == '-')
            continue;

        if (wantsCharOne)
            char1 = c;
        else {
            char2 = c;

            uint8_t combined = hexDigitToChar(char1) * 16 + hexDigitToChar(char2);
            data.push_back(combined);
        }

        wantsCharOne = !wantsCharOne;
    }
    this->data = data;
}

bool UUID::is16() const {
    return data.size() == 2;
}

bool UUID::is128() const {
    // lol
    return !is16();
}

uint16_t UUID::data16() const {
    uint8_t byte1 = data[0];
    uint8_t byte2 = data[1];
    return (byte2 << 8) | byte1;
}

const uint8_t* UUID::data128() const {
    return data.data();
}

std::string UUID::toString() const {
    if (is16()) {
        return "[16 bit uuid]";
    }

	char one[10], two[6], three[6], four[6], five[14];

	snprintf(one, 10, "%02x%02x%02x%02x",
		data[0], data[1], data[2], data[3]);
	snprintf(two, 6, "%02x%02x",
		data[4], data[5]);
	snprintf(three, 6, "%02x%02x",
		data[6], data[7]);
	snprintf(four, 6, "%02x%02x",
		data[8], data[9]);
	snprintf(five, 14, "%02x%02x%02x%02x%02x%02x",
		data[10], data[11], data[12], data[13], data[14], data[15]);
	const std::string sep("-");
	std::string out(one);

	out += sep + two;
	out += sep + three;
	out += sep + four;
	out += sep + five;

	return out;
}
