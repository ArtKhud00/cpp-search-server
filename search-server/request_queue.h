#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        std::vector<Document> res = server.FindTopDocuments(raw_query, document_predicate);
        requests_.push_back({ raw_query, !res.empty() });
        AddRequest(res);
        return res;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string query_words;
        bool isFoundsmth;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int zerores_count;
    const SearchServer& server;

    void AddRequest(std::vector<Document>& docs);
};

