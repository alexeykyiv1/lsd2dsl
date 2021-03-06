#include "SystemDictionaryDecoder.h"
#include "BitStream.h"
#include "tools.h"

namespace dictlsd {

SystemDictionaryDecoder::SystemDictionaryDecoder(bool xoring)
    : _xoring(xoring) { }

void SystemDictionaryDecoder::Read(IBitStream *bstr) {
    XoringStreamAdapter adapter(bstr);
    if (_xoring) {
        bstr = &adapter;
    }
    int len = bstr->read(32);
    _prefix = readUnicodeString(bstr, len, true);
    _articleSymbols = readSymbols(bstr);
    _headingSymbols = readSymbols(bstr);
    _ltArticles.Read(*bstr);
    _ltHeadings.Read(*bstr);

    _ltPostfixLengths.Read(*bstr);
    bstr->read(32);
    _ltPrefixLengths.Read(*bstr);

    _huffman1Number = bstr->read(32);
    _huffman2Number = bstr->read(32);
}

void SystemDictionaryDecoder::DecodeHeading(IBitStream *bstr, unsigned len, std::u16string &res) {
    res.clear();
    unsigned symIdx;
    for (size_t i = 0; i < len; i++) {
        _ltHeadings.Decode(*bstr, symIdx);
        unsigned sym = _headingSymbols.at(symIdx);
        assert(sym <= 0xffff);
        res += (char16_t)sym;
    }
}

bool SystemDictionaryDecoder::DecodeArticle(IBitStream *bstr, std::u16string &res) {
    XoringStreamAdapter adapter(bstr);
    if (_xoring) {
        bstr = &adapter;
    }
    unsigned maxlen = bstr->read(16);
    if (maxlen == 0xFFFF) {
        maxlen = bstr->read(32);
    }
    res.clear();
    unsigned symIdx;
    while ((unsigned)res.length() < maxlen) {
        _ltArticles.Decode(*bstr, symIdx);
        unsigned sym = _articleSymbols.at(symIdx);
        if (sym - 0x80 >= 0x10000) {
            if (sym <= 0x3F) {
                unsigned startIdx = bstr->read(BitLength(_prefix.length()));
                unsigned len = sym + 3;
                res += _prefix.substr(startIdx, len);
            } else {
                unsigned startIdx = bstr->read(BitLength(maxlen));
                unsigned len = sym - 0x3d;
                res += res.substr(startIdx, len);
            }
        } else {
            res += (char16_t)(sym - 0x80);
        }
    }
    return true;
}

bool SystemDictionaryDecoder::DecodePrefixLen(IBitStream &bstr, unsigned &len) {
    return _ltPrefixLengths.Decode(bstr, len);
}

bool SystemDictionaryDecoder::DecodePostfixLen(IBitStream &bstr, unsigned &len) {
    return _ltPostfixLengths.Decode(bstr, len);
}

bool SystemDictionaryDecoder::ReadReference1(IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman1Number);
}

bool SystemDictionaryDecoder::ReadReference2(IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman2Number);
}

std::u16string SystemDictionaryDecoder::Prefix() {
    return _prefix;
}

}
