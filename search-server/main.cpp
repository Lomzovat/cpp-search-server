#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;


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
void SortByRelevance() {

    SearchServer server;
    const int doc_id_1 = 1;
    const int doc_id_2 = 2;
    const int doc_id_3 = 3;
    const string line_1 = "cat with green eyes"s;
    const string line_2 = "iguana with eyes"s;
    const string line_3 = "iguana with red eyes"s;
    const vector<int> ratings = { 1, 1, 1 };
    server.AddDocument(doc_id_1, line_1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, line_2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_3, line_3, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("iguana red eyes "s);
    ASSERT_EQUAL(found_docs.at(0).id, doc_id_3);
    ASSERT_EQUAL(found_docs.at(1).id, doc_id_2);
    ASSERT_EQUAL(found_docs.at(2).id, doc_id_1);
}

//Вычисление рейтинга
void CalculateAverageRating() {
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
void SortByPredicate() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "iguana in the house"s, DocumentStatus::IRRELEVANT, { -1, -2, -3 });
    const auto found_docs = server.FindTopDocuments("in the"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    {
        ASSERT_EQUAL(found_docs[0].id, 42);
    }
}

//Поиск по статусу
void TestByStatus() {

    SearchServer server;

    server.AddDocument(42, "parrot in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "cat in the small hall", DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(44, "dog in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(45, "cat in the dress", DocumentStatus::REMOVED, { 1, 2, 3 });
    server.AddDocument(46, "iguana in the hat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 45);
}

void TestCorrectRelevance() {
    SearchServer server;
    server.SetStopWords("in the"s);


    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    const auto found_docs = server.FindTopDocuments("cat city"s);
    ASSERT(found_docs.at(0).rating >= 1);
    ASSERT(found_docs.at(1).rating <= 0.2);
}




// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWordsFromSearch();
    TestAddedDocumentsByRequestWord();
    TestMatchedDocuments();
    SortByRelevance();
    CalculateAverageRating();
    SortByPredicate();
    TestByStatus();
    TestCorrectRelevance();

}
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}