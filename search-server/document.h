#pragma once

#include <set>
#include <string>
#include <iostream>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DELTA = 1e-06;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

std::ostream& operator<< (std::ostream& out, const Document& doc);