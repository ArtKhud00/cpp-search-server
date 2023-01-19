#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : zero_result_count(0), server(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    std::vector<Document> res = server.FindTopDocuments(raw_query, status);
    requests_.push_back({ raw_query, !res.empty() });
    AddRequest(res);
    return res;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    std::vector<Document> res = server.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    requests_.push_back({ raw_query, !res.empty() });
    AddRequest(res);
    return res;
}

int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return zero_result_count;
}

void RequestQueue::AddRequest(std::vector<Document>& docs) {
    if (docs.empty()) {
        ++zero_result_count;
    }
    if (requests_.size() > min_in_day_) {
        if (!requests_.front().is_found) {
            --zero_result_count;
        }
        requests_.pop_front();
    }
}