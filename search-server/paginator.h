#pragma once

#include <algorithm>

using namespace std;

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator range_begin, Iterator range_end)
            :begin_(range_begin)
            , end_(range_end)
            , size_(distance(range_begin, range_end))
    {
    }
    Iterator begin() const {
        return begin_;
    }
    Iterator end() const {
        return end_;
    }
    size_t size() {
        return size_;
    }
private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size)
            : pages_()
    {
        int items = distance(range_begin, range_end);
        if (items <= page_size) {
            AddPage(range_begin, range_end);
        } else {
            int count_pages = items/page_size;
            for (int i = 0; i < count_pages; ++i) {
                AddPage(range_begin, range_begin + page_size);
                advance(range_begin, page_size);
            }
            if (range_begin != range_end) {
                AddPage(range_begin, range_end);
            }
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
private:
    vector<IteratorRange<Iterator>> pages_;
    void AddPage (Iterator range_begin, Iterator range_end) {
        auto page = IteratorRange<Iterator>(range_begin, range_end);
        pages_.push_back(page);
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
ostream& operator<< (ostream& out, const IteratorRange<Iterator>& it_ranges) {
    for (auto it = it_ranges.begin(); it_ranges.end() != it; ++it) {
        out << *it;
    }
    return out;
}