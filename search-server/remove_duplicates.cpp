#include "remove_duplicates.h"
using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {


    std::set<int> id_removed;
    std::map<std::set<std::string>, int> words_and_id;
    for (const int document_id : search_server) {
        std::map<std::string, double> all_words;

        all_words = search_server.GetWordFrequencies(document_id);
        std::set<std::string> unique_words;

        for (auto [word, _] : all_words) {
            unique_words.insert(word);
        }

        if (words_and_id.count(unique_words)) {
            id_removed.insert(document_id);
        }
        else {
            words_and_id.insert(std::pair{ unique_words, document_id });
        }
    }
    for (auto elem : id_removed) {
        std::cout << "Found duplicate document id " << elem << std::endl;
        search_server.RemoveDocument(elem);
    }
}