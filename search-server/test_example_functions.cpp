#include "test_example_functions.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
	search_server.AddDocument(document_id, document, status, ratings);
}