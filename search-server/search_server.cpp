#include "search_server.h"


using namespace std;


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(std::string_view(stop_words_text)))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    deq_string_.push_back(std::string(document));

    const auto words = SplitIntoWordsNoStop(std::string_view(deq_string_.back()));

    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&,
    std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

// итератор на начало документов
std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

// итератор на конец документов
std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

// метод получения частот слов по id документа
const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> res;
    if (document_to_word_freqs.count(document_id) != 0) {
        return document_to_word_freqs.at(document_id);
    }
    return res;
}

//  метод удаления документов из поискового сервера
void SearchServer::RemoveDocument(int document_id) {
    documents_.erase(document_id);
    document_ids_.erase(std::find(document_ids_.begin(), document_ids_.end(), document_id));

    for (auto& [word, _] : document_to_word_freqs[document_id]) {
        auto erase_word = word_to_document_freqs_[word].find(document_id);
        word_to_document_freqs_[word].erase(erase_word);
    }
    document_to_word_freqs.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {

    documents_.erase(document_id);
    document_ids_.erase(std::find(std::execution::par, document_ids_.begin(), document_ids_.end(), document_id));

    std::vector<std::string_view> strings_pointers(document_to_word_freqs.at(document_id).size());
    const auto& document = document_to_word_freqs.at(document_id);
    transform(std::execution::par,
        document.begin(),
        document.end(),
        strings_pointers.begin(),
        [](auto& word_to_freqs) {
            return string_view(word_to_freqs.first);
        });
    auto del = [this, document_id](const auto s) {
        word_to_document_freqs_.at(std::string(s)).erase(document_id);
    };

    for_each(std::execution::par, strings_pointers.begin(), strings_pointers.end(), del);
    document_to_word_freqs.erase(document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const {
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Document does not exist");
    }
    const auto query = ParseQuery(raw_query, true);

    auto status = documents_.at(document_id).status;
    std::vector<std::string_view> matched_words;

    for (const auto word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            //matched_words.clear();
            //break;
            return  { matched_words, status };
        }
    }

    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query,
    int document_id) const {

    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Document does not exist");
    }

    const auto query = ParseQuery(raw_query, false);
    const auto status = documents_.at(document_id).status;
    // если есть минус слова
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&, document_id](const auto word) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            //matched_words.clear();
            return true;
        }
        return false;
        }) == true) {
        std::vector<std::string_view> no_words;
        return { no_words, status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&, document_id](auto word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return false;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return true;
        }
        return false;
        });

    std::sort(std::execution::par, matched_words.begin(), words_end);
    auto it = std::unique(std::execution::par, matched_words.begin(), words_end);
    matched_words.erase(it, matched_words.end());
    return { matched_words, status };
}



bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (auto& word : SplitIntoWords(text)) {
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
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    auto word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool del_dubl) const {
    Query result;
    for (auto word : SplitIntoWords(text)) {
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
    // удаление дубликатов в векторах плюс и минус слов
    if (del_dubl) {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        std::sort(result.plus_words.begin(), result.plus_words.end());
        auto last1 = std::unique(result.minus_words.begin(), result.minus_words.end());
        auto last2 = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.minus_words.erase(last1, result.minus_words.end());
        result.plus_words.erase(last2, result.plus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}