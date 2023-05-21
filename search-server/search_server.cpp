#include <numeric>
#include "search_server.h"

void SearchServer::AddDocument(int document_id,
    std::string_view document,
    DocumentStatus status,
    const std::vector<int>& ratings) {

    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document ID"s);
    }

    auto [doc_id_, doc_data_] = documents_.emplace(document_id,
        DocumentData{
            std::string(document),
            ComputeAverageRating(ratings),
            status
        });
    document_ids_.insert(document_id);

    const auto words = SplitIntoWordsNoStop(doc_id_->second.data_string_);

    const double inv_word_count = 1.0 / words.size();

    for (auto word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        docs_ids_to_word_freqs_[document_id][word] += inv_word_count;
    }
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
    std::string_view raw_query,
    DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq,
        raw_query,
        [status](int document_id,
            DocumentStatus document_status,
            int rating) {
                return document_status == status;
        }
    );
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&,
    std::string_view raw_query,
    DocumentStatus status) const {
    return FindTopDocuments(std::execution::par,
        raw_query,
        [status](int document_id,
            DocumentStatus document_status,
            int rating) {
                return document_status == status;
        }
    );
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq,
        raw_query,
        DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
    std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq,
        raw_query,
        DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&,
    std::string_view raw_query) const {
    return FindTopDocuments(std::execution::par,
        raw_query,
        DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int> ::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int> ::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> emptyes;
    return (!docs_ids_to_word_freqs_.count(document_id))
        ? emptyes
        : docs_ids_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq,
        document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&,
    int document_id) {

    if (document_ids_.find(document_id) != document_ids_.end()) {
        for (auto& [word, _] : docs_ids_to_word_freqs_[document_id]) {
            auto erase_word = word_to_document_freqs_[word].find(document_id);
            word_to_document_freqs_[word].erase(erase_word);
        }
    }

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    docs_ids_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {

    std::vector<std::string_view> bin(docs_ids_to_word_freqs_.at(document_id).size());
    std::transform(std::execution::par, docs_ids_to_word_freqs_.at(document_id).begin(), docs_ids_to_word_freqs_.at(document_id).end(), bin.begin(), [](auto word_freq) {
        return word_freq.first;
        });

    std::for_each(std::execution::par, bin.begin(), bin.end(), [this, document_id](const auto& word) {
        word_to_document_freqs_.at(word).erase(document_id);
        });

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    docs_ids_to_word_freqs_.erase(document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const {
    return MatchDocument(std::execution::seq,
        raw_query,
        document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
    std::string_view raw_query,
    int document_id) const {

    if ((document_id < 0) || (documents_.count(document_id) <= 0)) {
        throw std::invalid_argument("Document ID doesn't exist"s);
    }

    const auto result = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    for (auto word : result.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { {}, documents_.at(document_id).status };
        }
    }

    for (auto word : result.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
    std::string_view raw_query,
    int document_id) const {

    if ((document_id < 0) || (documents_.count(document_id) <= 0)) {
        throw std::invalid_argument("Document ID doesn't exist"s);
    }

    const auto& result = ParseQuery(raw_query);

    const auto& check = [this, document_id](std::string_view word) {
        const auto temp = word_to_document_freqs_.find(word);
        return temp != word_to_document_freqs_.end() &&
            temp->second.count(document_id);
    };

    if (std::any_of(std::execution::par,
        result.minus_words.begin(),
        result.minus_words.end(),
        check)) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(result.plus_words.size());

    auto end = std::copy_if(std::execution::par,
        result.plus_words.begin(),
        result.plus_words.end(),
        matched_words.begin(),
        check);

    std::sort(matched_words.begin(), end);
    end = std::unique(std::execution::par, matched_words.begin(),end);

    matched_words.erase(end, matched_words.end());
    return { matched_words,
            documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        }
    );
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;

    for (std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(),
        ratings.end(),
        0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;

    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view& text) const {
    Query result;

    for (std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    std::sort(std::execution::par, result.minus_words.begin(), result.minus_words.end());
    std::sort(std::execution::par, result.plus_words.begin(),result.plus_words.end());

    result.minus_words.erase(std::unique(result.minus_words.begin(),
        result.minus_words.end()),
        result.minus_words.end());
    result.plus_words.erase(std::unique(result.plus_words.begin(),
        result.plus_words.end()),
        result.plus_words.end());

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}