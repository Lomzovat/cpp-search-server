#pragma once

#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include "document.h"

using namespace std::string_literals;

template <typename IteratorRanges>
class IteratorRange {
public:
    explicit IteratorRange(IteratorRanges begin, IteratorRanges end) :
        begin_(begin),
        end_(end),
        size_(distance(begin, end)) {}

    IteratorRanges begin() const {
        return begin_;
    }
    IteratorRanges end() const {
        return end_;
    }
    size_t size() const {
        return size_;
    }
private:
    IteratorRanges begin_;
    IteratorRanges end_;
    size_t size_;
};

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ document_id = "s << document.id
        << ", relevance = "s << document.relevance
        << ", rating = "s << document.rating << " }"s;
    return out;
}

template <typename Outer>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Outer>& sheet) {
    auto temp = sheet.begin();
    while (temp != sheet.end()) {
        out << *temp;
        ++temp;
    }
    return out;
}

template <typename P>
class Paginator {
public:
    Paginator(const P& result_begin, const P& result_end, size_t size_of_sheet) {
        auto full_size = distance(result_begin, result_end);
        P temp = result_begin;
        for (auto i = 0; i < full_size / size_of_sheet; ++i) {
            sheets.push_back(IteratorRange<P>(temp, temp + size_of_sheet));
            temp = temp + size_of_sheet;
        }
        if (temp != result_end) {
            sheets.push_back(IteratorRange<P>(temp, result_end));
        }
    }
    auto begin() const {
        return sheets.begin();
    }
    auto end() const {
        return sheets.end();
    }
    size_t size() {
        return sheets.size();
    }
private:
    std::vector<IteratorRange<P>> sheets;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}