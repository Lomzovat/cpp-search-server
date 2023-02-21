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
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "iguana in the house"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto found_docs = server.FindTopDocuments("in the city"s);

        ASSERT_EQUAL(found_docs[0].id, 42);
    }
}

//Вычисление рейтинга
void CalculateAverageRating() {
    SearchServer server;
    const vector<int> ratings = { 1, 2, 3 };
    const int average = (1 + 2 + 3) / 3;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, ratings);

    {
        const auto found_docs = server.FindTopDocuments("cat in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].rating, average);
    }
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
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "iguana in the house"s, DocumentStatus::IRRELEVANT, { -1, -2, -3 });
    const auto found_docs = server.FindTopDocuments("in the"s, DocumentStatus::ACTUAL);

    {
        ASSERT_EQUAL(found_docs[0].id, 42);
    }
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
}
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}