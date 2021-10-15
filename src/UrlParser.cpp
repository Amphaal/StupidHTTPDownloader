// StupidHTTPDownloader
// Really stupid library to download HTTP(S) content
// Copyright (C) 2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "UrlParser.h"

#include <spdlog/spdlog.h>

UrlParser::UrlParser(std::string_view rawUrlView) {
    // find scheme separator
    std::string schemeSeparator("://");
    auto findSS = rawUrlView.find(schemeSeparator);
    
    // if path separator is found without scheme or not found at all, return
    if (findSS == std::string::npos || findSS == 0) {
        return;
    }

    // define scheme
    this->_scheme = rawUrlView.substr(0, findSS);

    // is considered valid
    this->_isValid = true;

    // find first / or ? to determine host bounds
    auto afterSchemeSeparatorPos = findSS + schemeSeparator.length();
    auto findFirstSlash = rawUrlView.find("/", afterSchemeSeparatorPos);
    auto findFirstIPoint = rawUrlView.find("?", afterSchemeSeparatorPos);

    // no bounds fond, consider host being the remaning of string
    if (findFirstSlash == std::string::npos && findFirstIPoint == std::string::npos) {
        this->_host = rawUrlView.substr(afterSchemeSeparatorPos);
        return;
    }

    // if a slash is found after scheme
    if(findFirstSlash != std::string::npos) {
        _noPathInitiator = false;
    }

    // pick which separator came first
    auto firstHostSeparatorPos = findFirstSlash > findFirstIPoint ? findFirstIPoint : findFirstSlash;

    // determine host part
    auto pathStartIndex = firstHostSeparatorPos - afterSchemeSeparatorPos;
    this->_host = rawUrlView.substr(afterSchemeSeparatorPos, pathStartIndex);

    // else is path + query
    this->_pathAndQuery = rawUrlView.substr(firstHostSeparatorPos);
}

bool UrlParser::isValid() const {
    return this->_isValid;
}

std::string UrlParser::host() const {
    return std::string { this->_host };
}

std::string UrlParser::scheme() const {
    return std::string{ this->_scheme };
}

bool UrlParser::isHTTPS() const {
    return this->_scheme == "https";
}

std::string UrlParser::pathAndQuery() const {
    std::string out { this->_pathAndQuery };
    if (_noPathInitiator) out = "/" + out;
    return out;
}

UrlQuery::UrlQuery() { }
UrlQuery::UrlQuery(const UrlQuery::Key &key, const UrlQuery::SubQuery &subQuery) : UrlQuery(subQuery) {
    this->_selfKey = key;
}
UrlQuery::UrlQuery(const std::string_view &query) {
    this->_wholeQuery = query;

    // setup for search
    auto keyFindStart = this->_wholeQuery.begin();
    auto valFindStart = keyFindStart;
    UrlQuery::Key currentKey;
    enum {
        FIND_KEY,
        FIND_VALUE
    };
    auto finderFunc = FIND_KEY;

    // search
    for (auto currentChar = keyFindStart; currentChar != this->_wholeQuery.end(); currentChar++) {
        switch (finderFunc) {
            case FIND_KEY: {
                if (*currentChar != *"=") continue;

                currentKey = std::string(keyFindStart, currentChar);

                valFindStart = currentChar + 1;
                finderFunc = FIND_VALUE;
            }
            break;

            case FIND_VALUE: {
                if (*currentChar != *"&") continue;

                UrlQuery::SubQuery subq(&*valFindStart, currentChar - valFindStart);  // &* to allow MSVC (https://github.com/abseil/abseil-cpp/issues/161)
                this->_subqueries.emplace(currentKey, subq);

                keyFindStart = currentChar + 1;
                finderFunc = FIND_KEY;
            }
            break;
        }
    }

    // end FIND_VALUE
    if (finderFunc == FIND_VALUE) {
        UrlQuery::SubQuery subq(&*valFindStart, this->_wholeQuery.end() - valFindStart);   // &* to allow MSVC (https://github.com/abseil/abseil-cpp/issues/161)
        this->_subqueries.emplace(currentKey, subq);
    }
}

std::string UrlQuery::key() const {
    return this->_selfKey;
}

bool UrlQuery::hasSubqueries() const {
    return this->_subqueries.size();
}

UrlQuery UrlQuery::operator[](const UrlQuery::Key &key) const {
    auto keyFound = this->_subqueries.find(key);
    if (keyFound == this->_subqueries.end()) return UrlQuery();
    return UrlQuery(key, keyFound->second);
}

std::vector<UrlQuery> UrlQuery::subqueries() const {
    std::vector<UrlQuery> out;
    for (auto &[k, v] : this->_subqueries) {
        out.emplace(out.end(), k, v);
    }
    return out;
}

std::string UrlQuery::percentDecoded() const {
    return Url::decode(this->undecoded());
}

std::string UrlQuery::undecoded() const {
    return std::string { this->_wholeQuery };
}

std::string Url::decode(const std::string & sSrc) {
    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
    const int SRC_LEN = sSrc.length();
    const unsigned char * const SRC_END = pSrc + SRC_LEN;
    const unsigned char * const SRC_LAST_DEC = SRC_END - 2;  // last decodable '%'

    char * const pStart = new char[SRC_LEN];
    char * pEnd = pStart;

    while (pSrc < SRC_LAST_DEC) {
        if (*pSrc == '%') {
            char dec1, dec2;
            if (-1 != (dec1 = _HEX2DEC[*(pSrc + 1)]) && -1 != (dec2 = _HEX2DEC[*(pSrc + 2)])) {
                *pEnd++ = (dec1 << 4) + dec2;
                pSrc += 3;
                continue;
            }
        }

        *pEnd++ = *pSrc++;
    }

    // the last 2- chars
    while (pSrc < SRC_END)
        *pEnd++ = *pSrc++;

    std::string sResult(pStart, pEnd);
    delete [] pStart;
    return sResult;
}

std::string Url::encode(const std::string & sSrc) {
    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
    const int SRC_LEN = sSrc.length();
    unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
    unsigned char * pEnd = pStart;
    const unsigned char * const SRC_END = pSrc + SRC_LEN;

    for (; pSrc < SRC_END; ++pSrc) {
        if (_SAFE[*pSrc]) {
            *pEnd++ = *pSrc;
        } else {
            // escape this char
            *pEnd++ = '%';
            *pEnd++ = _DEC2HEX[*pSrc >> 4];
            *pEnd++ = _DEC2HEX[*pSrc & 0x0F];
        }
    }

    std::string sResult(
        reinterpret_cast<char *>(pStart),
        reinterpret_cast<char *>(pEnd)
    );
    delete [] pStart;
    return sResult;
}

// Only alphanum is safe.
const char Url::_SAFE[256] = {
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

const char Url::_HEX2DEC[256] = {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

const char Url::_DEC2HEX[16 + 1] = "0123456789ABCDEF";
