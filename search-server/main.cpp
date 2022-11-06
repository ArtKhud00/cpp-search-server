#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
    int id;
    double relevance;
};



class SearchServer {
public:
    void SetStopWords(const string& text) {
        if (!text.empty()) {
            for (const string& word : SplitIntoWords(text)) {
                stop_words_.insert(word);
            }
        }
        else {
            stop_words_ = {};
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        int words_size = static_cast<int>(words.size());

        if (!words.empty()) {
            double TF = 1.0 / words_size;
            for (const string& word : words) {
                if (!stop_words_.count(word)) {
                    word_to_document_freqs_[word][document_id] += TF;
                }
            }
            ++document_count_;
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> minuswords;
        set<string> pluswords;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;

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

    QueryWord ParseQueryWord(string text)const {
        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minuswords.insert(query_word.data);
                }
                else {
                    query.pluswords.insert(query_word.data);
                }
            }
        }
        return query;
    }

    vector<Document>  FindAllDocuments(const Query& query) const {
        if (word_to_document_freqs_.empty() || query.pluswords.empty()) { return {}; }
        map<int, double> document_to_relevance;
        vector <Document> matched_documents = {};
        for (const auto& word : query.pluswords) {

            if (word_to_document_freqs_.count(word) != 0) {
                int size = word_to_document_freqs_.at(word).size();
                double IDF = static_cast<double>(document_count_) / size;
                IDF = log(IDF);
                for (const auto& [id, TF] : word_to_document_freqs_.at(word)) {
                    document_to_relevance[id] += IDF * TF;
                }
            }
        }
        if (!query.minuswords.empty() && !document_to_relevance.empty()) {
            for (const auto& word : query.minuswords) {
                if (word_to_document_freqs_.count(word) != 0) {
                    for (const auto& [id, rel] : word_to_document_freqs_.at(word)) {
                        document_to_relevance.erase(id);
                    }
                }
            }
        }
        for (const auto& [id, rel] : document_to_relevance) {
            matched_documents.push_back({ id, rel });
        }
        return matched_documents;
    }
};
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}

