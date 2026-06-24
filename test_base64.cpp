#include <cstdio>
#include <vector>
#include <string>

std::vector<unsigned char> base64UrlDecode(const std::string& input)
{
    static const signed char decodeTable[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,62,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6,  7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22, 23,24,25,-1,-1,-1,-1,63,
        -1,26,27,28,29,30,31,32, 33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48, 49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    };

    std::vector<unsigned char> result;
    result.reserve((input.size() * 3) / 4 + 1);

    unsigned int buffer = 0;
    int bitsCollected = 0;

    for (auto ch : input)
    {
        if (ch == '=') break;
        int value = decodeTable[static_cast<unsigned char>(ch)];
        if (value < 0) continue;

        buffer = (buffer << 6) | static_cast<unsigned int>(value);
        bitsCollected += 6;

        if (bitsCollected >= 8)
        {
            bitsCollected -= 8;
            result.push_back(static_cast<unsigned char>((buffer >> bitsCollected) & 0xFF));
        }
    }

    return result;
}

int main() {
    std::string sig = "Jog_2QbpPwPIESSQntN3gz3n_gHhQm9J2A3_iyntwUq81wR--M-7OFugnAveLAWmEUsGw8M9SfDXMuO_ZF2fCA";
    auto r = base64UrlDecode(sig);
    printf("size: %zu\n", r.size());
    printf("input len: %zu\n", sig.size());
    
    // Test with a simple 4-char group
    auto r2 = base64UrlDecode("AAAA");
    printf("AAAA size: %zu\n", r2.size());
    
    // Test 8 chars = 6 bytes
    auto r3 = base64UrlDecode("AAAAAAAA");
    printf("AAAAAAAA size: %zu\n", r3.size());
    
    return 0;
}
