#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> id_to_remove;
    std::map<std::set<string_view>, int> unique_words_docid;
        for (const int document_id : search_server) {
/*             std::map<string, double> all_words = search_server.GetWordFrequencies(document_id);
            std::set<string> unique_words_in_doc; */
			std::map<string_view, double> all_words = search_server.GetWordFrequencies(document_id);
            std::set<string_view> unique_words_in_doc;
			
            for (auto& [word, _] : all_words) {
                unique_words_in_doc.insert(word);
            }
            if (unique_words_docid.count(unique_words_in_doc) != 0) {
                id_to_remove.insert(document_id);
            }
            else {
                unique_words_docid.insert({ unique_words_in_doc,document_id });
            }

        }
        for (auto& id : id_to_remove) {
            std::cout << "Found duplicate document id "s << id << std::endl;
            search_server.RemoveDocument(id);
        }
}