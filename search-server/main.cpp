#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}



struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;

    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id),
        relevance(relevance),
        rating(rating)
    {
    }
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

   

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& single : stop_words) {
            if (!IsValidWord(single)) {
                throw invalid_argument("Invalid symbols are used"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, const string& document,
        DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("Forbidden to add a document with negative id"s);
        }
        if (documents_.count(document_id) > 0) {
            throw invalid_argument("Forbidden to add a document with the same id"s);
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("Invalid symbols are used"s);
        }
        

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
            documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
            index_id_.push_back(document_id);
    }
    


    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        Predicate predicate) const {

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {

        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });

    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentId(int index) const {
        if ((index > index_id_.size()) || (index < 0)) {
            throw out_of_range("Invalid document id"s);
        }
        else {
            return index_id_.at(index);
        }
    }

    size_t GetDocumentCount() const {
        return documents_.size();
    }


    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        if (document_id < 0) {
            throw invalid_argument("Forbidden to add a document with negative id"s);
        }
        const Query query = ParseQuery(raw_query);

        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    
    vector<int> index_id_;

    static bool IsValidWord(const string& word) {

        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (!IsValidWord(text)) {
            throw invalid_argument("Invalid symbols are used"s);
        }

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
            if (text.empty()) {
                throw invalid_argument("There is no text after '-'"s);

            }

            if (text[0] == '-') {
                throw invalid_argument("Double symbol '-' is not allowed"s);
            }
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0)
                continue;
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& found_document = documents_.at(document_id);
                if (predicate(document_id, found_document.status, found_document.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(const Iterator begin, const Iterator end) : begin_(begin), end_(end) {}

    auto begin() const {
        return begin_;
    }
    auto end() const {
        return end_;
    }
    int size() const {
        return (distance(begin_, end_));
    }

private:
    Iterator begin_;
    Iterator end_;
};


template <typename Iterator>
class Paginator
{
public:

    Paginator(const Iterator begin, const Iterator end, int size)
        : page_size_(size) {
        Iterator page_begin = begin;
        Iterator page_end = begin;

        while (distance(page_end, end) > size) {
            advance(page_end, size);
            pages_.push_back(IteratorRange(page_begin, page_end));
            page_begin = page_end;
        }

        if (page_begin != end) {
            pages_.push_back(IteratorRange(page_end, end));
        }
    }

    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    size_t size() const {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
    int page_size_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}


template<typename Iterator>
ostream& operator <<(ostream& os, const IteratorRange<Iterator>& page) {
    for (Iterator it = page.begin(); it != page.end(); ++it) {
        os << "{ "s
            << "document_id = "s << it->id << ", "s
            << "relevance = "s << it->relevance << ", "s
            << "rating = "s << it->rating << " }"s;
    }
    return os;
}



int main() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // ¬ыводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
}