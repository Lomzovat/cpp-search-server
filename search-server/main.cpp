#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

const double EPSILON = 1e-6;

// -------- Начало модульных тестов поисковой системы ----------
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов



// Минус-слова
void TestExcludeMinusWordsFromSearch() {

    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "black iguana in the central park", DocumentStatus::ACTUAL, { 1, 2, 3 });
    {
        const auto found_docs = server.FindTopDocuments("park -central"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
    {
        const auto found_docs = server.FindTopDocuments("city -cat"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
}

//Добавление документов
void TestAddedDocumentsByRequestWord() {
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
        server.AddDocument(43, "terribal gnome uder my bed", DocumentStatus::ACTUAL, { 3, 2 ,1 });
        ASSERT_EQUAL(server.GetDocumentCount(), 2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc_to_compare = found_docs[0];
        ASSERT_EQUAL(doc_to_compare.id, 42);
    }
}
//Матчинг
void TestMatchedDocuments() {
    SearchServer server;
    server.SetStopWords("at with no the"s);
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "dog at the school"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto [matched_words, status] = server.MatchDocument("city cat"s, 42);
        const vector<string> expected_result = { "cat"s, "city"s };
        ASSERT_EQUAL(expected_result, matched_words);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("dog -school"s, 43);
        const vector<string> expected_result = {};
        ASSERT_EQUAL(expected_result, matched_words);
        ASSERT_HINT(matched_words.empty(), "Minus words must be excluded from search"s);
    }
}
//Сортировка по релевантности
void TestSortByRelevance() {

    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = { 1, 2, 3 };
    const int doc_id1 = 45;
    const string content1 = "iguana in the park"s;
    const string content2 = "parrot in the park"s;
    const vector<int> ratings1 = { 3, 4, 5 };

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("iguana walks in city park"s);
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_HINT(doc0.relevance > doc1.relevance, "неверная сортировка"s);
    }
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content2, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("iguana walks in city park"s);
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT(doc0.relevance - doc1.relevance < EPSILON);
        ASSERT(doc0.rating > doc1.rating);
    }
}


//Вычисление рейтинга
void TestCalculateAverageRating() {
    SearchServer server;

    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in our small hall"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the hall"s, DocumentStatus::ACTUAL, { -1, 4, 6 });
    server.AddDocument(3, "our dog in the hall"s, DocumentStatus::ACTUAL, { -3, -2, -1 });

    const int average_1 = 2;
    const int average_2 = 3;
    const int average_3 = -2;

    const auto found_docs = server.FindTopDocuments("our dog in the hall"s);

    ASSERT_EQUAL(found_docs.at(0).rating, average_3);
    ASSERT_EQUAL(found_docs.at(1).rating, average_2);
    ASSERT_EQUAL(found_docs.at(2).rating, average_1);

}




//Фильтрация предикатом
void TestFilterByPredicate{
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "iguana in the house"s, DocumentStatus::IRRELEVANT, { -1, -2, -3 });
    const auto found_docs = server.FindTopDocuments("in the"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    {
        ASSERT_EQUAL(found_docs[0].id, 42);
    }
}




void TestByStatus() {
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = { 1, 2, 3 };
    const int doc_id1 = 43;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = { 3, 4, 5 };
    const int doc_id2 = 45;
    const string content2 = "bird flies in the sky up the city"s;
    const vector<int> ratings2 = { 1, 3, 7 };
    {
        SearchServer server;
        server.SetStopWords("in the and from has and"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::REMOVED, ratings1);
        const auto found_docs_banned = server.FindTopDocuments("dog walks in city park"s, DocumentStatus::BANNED);
        const auto found_docs_actual = server.FindTopDocuments("cat falled from sky in park"s, DocumentStatus::ACTUAL);
        const auto found_docs_removed = server.FindTopDocuments("bird has gone from cat and dog"s, DocumentStatus::REMOVED);
        const auto banned = found_docs_banned[0];
        const auto actual = found_docs_actual[0];
        const auto removed = found_docs_removed[0];
        ASSERT_EQUAL(found_docs_banned.size(), 1);
        ASSERT_EQUAL(found_docs_actual.size(), 1);
        ASSERT_EQUAL(found_docs_removed.size(), 1);
        ASSERT_EQUAL(banned.id, doc_id1);
        ASSERT_EQUAL(actual.id, doc_id0);
        ASSERT_EQUAL(removed.id, doc_id2);
    }
}



void TestCorrectRelevance() {
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = { 1, 2, 3 };
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = { 3, 4, 5 };
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("dog walks in city park"s);
        double city = 0.693147;
        double dog = 0.693147;
        double park = 0.693147;
        double IDTF0 = 0.5 * city;
        double IDTF1 = (0.5 * (dog + park));
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT(abs(doc0.relevance - IDTF1) < EPSILON);
        ASSERT(abs(doc1.relevance - IDTF0) < EPSILON);
        ASSERT_HINT(doc0.relevance > doc1.relevance, "неверная сортировка"s);
    }
}




// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWordsFromSearch();
    TestAddedDocumentsByRequestWord();
    TestMatchedDocuments();
    TestSortByRelevance();
    TestCalculateAverageRating();
    TestFilterByPredicate;
    TestByStatus();
    TestCorrectRelevance();

}
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}