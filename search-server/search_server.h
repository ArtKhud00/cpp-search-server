#pragma once
#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <execution>
#include <deque>


using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const size_t DICTIONARY_COUNT = 1000;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);


    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query,
        DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query, true);
        const double epsilon = 1e-6;
        auto matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [epsilon](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < epsilon) {
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

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&,
        std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query,
        DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query, true);
        const double epsilon = 1e-6;
        auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

        sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
            [epsilon](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < epsilon) {
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

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&,
        std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::iterator begin();
    std::set<int>::iterator end();

    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);

    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
        int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query,
        int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query,
        int document_id) const;


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    // for Add method
    std::deque<std::string> deq_string_;


    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool del_dubl) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        return FindAllDocuments(std::execution::seq, query, document_predicate);
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query,
        DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (auto& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query,
        DocumentPredicate document_predicate) const {
        ConcurrentMap<int, double> document_to_relevance(DICTIONARY_COUNT);

        for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [this, document_predicate, &document_to_relevance](auto word) {
            if (word_to_document_freqs_.count(word) > 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
            });

        for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance](auto word) {
            if (word_to_document_freqs_.count(word) > 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.Erase(document_id);
                }
            }
            });

        std::map<int, double> document_to_relevance_all_in_one = document_to_relevance.BuildOrdinaryMap();

        std::vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance_all_in_one.size());
        for (const auto [document_id, relevance] : document_to_relevance_all_in_one) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};