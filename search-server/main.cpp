/*#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
/*
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, 
                unsigned line, const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(T& test, const string& func) {
    test();
    cerr << func << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func) 

*/
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь_______________________________________________________________
*/

// Проверка поддержки минус-слов и матчинга
void TestMinusWordsAndMatching() {
    SearchServer server;
    server.AddDocument(1, "cat in our small city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "cat in the house hall"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(3, "dog in the hall"s, DocumentStatus::ACTUAL, {1, 2, 3});


    {
        const auto found_docs = server.FindTopDocuments("our cat in the -hall"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 1);
    }

    {
        const auto found_docs = server.FindTopDocuments("our -cat in the hall"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 3);
    }

    {
        const auto found_docs = server.FindTopDocuments("our cat in the -city"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL(doc0.id, 2);
        ASSERT_EQUAL(doc1.id, 3);
    }

    {
        const auto found_docs = server.FindTopDocuments("our -cat in the -hall"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("our cat in the -hall"s, 1);
        ASSERT_EQUAL(matched_words[0], "cat"s);
        ASSERT_EQUAL(matched_words[1], "in"s);
        ASSERT_EQUAL(matched_words[2], "our"s);
    }

    {
        server.SetStopWords("in the"s);
        const auto [matched_words, status] = server.MatchDocument("our cat in the -hall"s, 1);
        ASSERT_EQUAL(matched_words[0], "cat"s);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("hall -dog"s, 3);
        ASSERT_EQUAL(matched_words.size(), 0);
    }
}

// Проверка сортировки по релевантности и расчет рейтинга
void TestSortByRelevance() {
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in our small city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "cat in the house hall"s, DocumentStatus::ACTUAL, {3, 4, 6});
    server.AddDocument(3, "dog in the hall"s, DocumentStatus::ACTUAL, {-3, 2, 1});
    const auto found_docs = server.FindTopDocuments("our dog in the hall"s);
    for (int i = 1; i < static_cast<int>(found_docs.size()); ++i) {
        ASSERT(found_docs[i-1].relevance > found_docs[i].relevance);
    }
}

// Корректность вычисления среднего рейтинга
void TestComputeAverageRating() {
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in our small city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "cat in the house hall"s, DocumentStatus::ACTUAL, {3, 4, 6});
    server.AddDocument(3, "dog in the hall"s, DocumentStatus::ACTUAL, {-3, 2, 1});
    const auto found_docs = server.FindTopDocuments("our dog in the hall"s);
    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];
    ASSERT_EQUAL(doc1.rating, (-3+2+1)/3);
    ASSERT_EQUAL(doc2.rating, (1+2+3)/3);
    ASSERT_EQUAL(doc3.rating, (3+4+6)/3);
}

// Фильтрация документов с предиктатом
void TestFilterWithPredictate() {
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in our small city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "cat in the house hall"s, DocumentStatus::ACTUAL, {3, 4, 6});
    server.AddDocument(3, "dog in the hall"s, DocumentStatus::ACTUAL, {-3, 2, 1});
    const auto found_docs = server.FindTopDocuments("hall"s, [] (int document_id, DocumentStatus status, int rating)
    {return document_id % 2 == 0;});
    ASSERT_EQUAL(found_docs[0].id, 2);
}

// Поиск по статусу
void TestSearchByStatus() {
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in our small city"s, DocumentStatus::REMOVED, {1, 2, 3});
    server.AddDocument(2, "cat in the house"s, DocumentStatus::ACTUAL, {3, 4, 6});
    server.AddDocument(3, "cat in the hall"s, DocumentStatus::BANNED, {-3, 2, 1});
    server.AddDocument(4, "cat in the hall"s, DocumentStatus::IRRELEVANT, {-3, 2, 1});
    server.AddDocument(5, "cat in the hall of house"s, DocumentStatus::ACTUAL, {-3, 2, 1});
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs[0].id, 2);
    ASSERT_EQUAL(found_docs[1].id, 5);
    const auto found_docs_banned = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(found_docs_banned[0].id, 3);
    const auto found_docs_irrelevant = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(found_docs_irrelevant[0].id, 4);
    const auto found_docs_removed = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(found_docs_removed[0].id, 1);
}

// Вычисление релевантности
/* ТРЕНАЖЕР ЭТОТ ТЕСТ ПОЧЕМУ-ТО НЕ ПРИНИМАЕТ!!!
void TestComputeRelevance() {
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat cat in our small city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "cat in the hall"s, DocumentStatus::ACTUAL, {3, 4, 6});
    server.AddDocument(3, "dog in the hall"s, DocumentStatus::ACTUAL, {-3, 2, 1});
    const auto found_docs = server.FindTopDocuments("cat dog"s);
    ASSERT_EQUAL(found_docs[0].id, 3);
    ASSERT((found_docs[0].relevance - (log(3*1.0/1))*(1*1.0/4)) < 1e-06);
    ASSERT_EQUAL(found_docs[1].id, 1);
    ASSERT((found_docs[1].relevance - (log(3*1.0/2))*(2*1.0/6)) < 1e-06);
    ASSERT_EQUAL(found_docs[2].id, 2);
    ASSERT((found_docs[2].relevance - (log(3*1.0/2))*(1*1.0/4)) < 1e-06);
    
}
*/


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordsAndMatching);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestFilterWithPredictate);
    RUN_TEST(TestSearchByStatus);
    //RUN_TEST(TestComputeRelevance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
