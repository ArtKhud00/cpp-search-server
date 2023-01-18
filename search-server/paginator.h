#pragma once
#include <vector>
#include <iostream>
#include "document.h"

using namespace std::string_literals;

template <typename Iterrange>
class IteratorRange {
public:
    IteratorRange(Iterrange begin, Iterrange end) : begin_(begin), end_(end), size_(distance(begin, end)) {}

    Iterrange begin() const {
        return begin_;
    }

    Iterrange end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterrange begin_;
    Iterrange end_;
    size_t size_;
};


std::ostream& operator<<(std::ostream& out, const Document& doc) {
    out << "{ "s
        << "document_id = "s << doc.id << ", "s
        << "relevance = "s << doc.relevance << ", "s
        << "rating = "s << doc.rating << " }"s;
    return out;
}


template <typename ToPrint>
std::ostream& operator<< (std::ostream& out, const IteratorRange<ToPrint>& data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename pager>
class Paginator {
    // тело класса
public:

    Paginator(pager begin, pager end, size_t size) : page_size(size) {

        auto page_dist = distance(begin, end);
        auto page_begin = begin;
        for (int i = 0; i < page_dist / page_size; ++i)
        {
            pages_.push_back(IteratorRange<pager>(page_begin, next(page_begin, size)));
            advance(page_begin, size);
        }
        if (page_begin != end) {
            pages_.push_back(IteratorRange<pager>(page_begin, end));
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() {
        return pages_.size();
    }

private:
    int page_size;
    std::vector <IteratorRange<pager>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}