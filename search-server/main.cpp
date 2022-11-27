
// The test checks that the search engine excludes stop words when adding documents
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // First, make sure that searching for a word 
    // that is not on the stopword list finds the right document
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Then we make sure that the search for the same word included in the list of stop words 
    // returns an empty result
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

//The test checks that the search engine excludes documents containing minus - words
void TestExcludeDocumentsFromResultWithMinusWords() {
    SearchServer server;
    int doc1id, doc2id;
    doc1id = 25;
    doc2id = 42;
    server.AddDocument(doc1id, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(doc2id, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    //check without minus - words
    {
        const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s);
        ASSERT_EQUAL(found_docs.size(), 2);
    }
    //include a minus - word in the query
    {
        const auto found_docs = server.FindTopDocuments("-fluffy groomed cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_HINT(doc0.id != doc2id, "inappropriate document was found"s);
    }
}

//Search query compliance test
void TestMatchingDocuments() {
    SearchServer server;
    int doc1id, doc2id;
    doc1id = 0;
    doc2id = 1;
    server.AddDocument(doc1id, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(doc2id, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    {
        const auto [words, status] = server.MatchDocument("-fluffy groomed cat"s, doc1id);
        ASSERT_EQUAL(words.size(), 1);
    }
    {
        const auto [words, status] = server.MatchDocument("-fluffy groomed cat"s, doc2id);
        ASSERT_EQUAL_HINT(words.size(), 0, "there is some documents that were found"s);
    }
}

//Test for sorting found documents by relevance
void TestSortingByRelevance() {
    SearchServer server;
    int doc1id, doc2id, doc3id;
    doc1id = 0;
    doc2id = 1;
    doc3id = 2;
    server.AddDocument(doc1id, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(doc2id, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(doc3id, "groomed dog highlighted eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("fluffy groomed cat"s);
    ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance &&
        found_docs[0].relevance > found_docs[2].relevance &&
        found_docs[1].relevance > found_docs[2].relevance, "The order of relevances is incorrect"s);
}

//Document rating calculation test
void TestCalcAvgRating() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 4 };
    int sum = 0;
    for (const int rate : ratings) {
        sum += rate;
    }
    int ratting = sum / static_cast<int>(ratings.size());
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("in"s);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.rating, ratting, "The rating was calculated incorrectly");
}

//User predicate test
void TestUserPredicate() {
    SearchServer search_server;
    search_server.SetStopWords("and in on"s);
    search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "fluffy groomed cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "groomed dog highlighted eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
    {
        const auto found_docs = search_server.FindTopDocuments("fluffy groomed cat"s, 
                                                                [](int document_id, DocumentStatus status, int rating) 
                                                                { return document_id % 2 == 0; });
        for (auto& doc : found_docs) {
            ASSERT(doc.id % 2 == 0);
        }
    }
    {
        const auto found_docs1 = search_server.FindTopDocuments("fluffy groomed cat"s, 
                                                                 [](int document_id, DocumentStatus status, int rating) 
                                                                 { return rating > 1; });
        for (auto& doc : found_docs1) {
            ASSERT(doc.rating > 1);
        }
    }
    {
        const auto found_docs = search_server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) 
                                                                                        { return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(found_docs.size(), 1);
    }

}

//Test of searching for documents of a given status
void TestSearchExactStatus() {
    string query = "fluffy groomed cat"s;
    SearchServer search_server;
    search_server.SetStopWords("and in on"s);
    search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "groomed dog highlighted eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "groomed starling evgeny"s, DocumentStatus::BANNED, { 9 });
    {
        DocumentStatus status_need = DocumentStatus::ACTUAL;
        const auto found_docs = search_server.FindTopDocuments(query, status_need);
        for (auto& doc : found_docs) {
            const auto& [_, status] = search_server.MatchDocument(query, doc.id);
            ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(status_need));
        }
    }
    {
        DocumentStatus status_need = DocumentStatus::BANNED;
        const auto found_docs = search_server.FindTopDocuments(query, status_need);
        for (auto& doc : found_docs) {
            const auto& [_, status] = search_server.MatchDocument(query, doc.id);
            ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(status_need));
        }
    }
}

//Test of the correctness of the calculation of the relevance of the found documents
void TestCalcRelevance() {
    string query = "fluffy groomed cat"s;
    SearchServer search_server;
    search_server.AddDocument(0, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(1, "white groomed cat and fashionable collar"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(2, "groomed dog highlighted eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = search_server.FindTopDocuments(query);

    //Make calculations of relevance manually
    //Calculate IDF for each query word
    double IDF_fluffy = log(static_cast<double>(3) / 1);
    double IDF_groomed = log(static_cast<double>(3) / 2);
    double IDF_cat = log(static_cast<double>(3) / 2);
    //Calculate TF for each query word in documents
    double TF_fluffy_doc0 = 2.0 / 4, TF_groomed_doc0 = 0.0 / 4, TF_cat_doc0 = 1.0 / 4;
    double TF_fluffy_doc1 = 0.0 / 6, TF_groomed_doc1 = 1.0 / 6, TF_cat_doc1 = 1.0 / 6;
    double TF_fluffy_doc2 = 0.0 / 4, TF_groomed_doc2 = 1.0 / 4, TF_cat_doc2 = 0.0 / 4;
    //Calculate the final relevance for each document
    double calcrel_doc0 = IDF_fluffy * TF_fluffy_doc0 + IDF_groomed * TF_groomed_doc0
        + IDF_cat * TF_cat_doc0;
    double calcrel_doc1 = IDF_fluffy * TF_fluffy_doc1 + IDF_groomed * TF_groomed_doc1
        + IDF_cat * TF_cat_doc1;
    double calcrel_doc2 = IDF_fluffy * TF_fluffy_doc2 + IDF_groomed * TF_groomed_doc2
        + IDF_cat * TF_cat_doc2;
    //Compare the results
    ASSERT_EQUAL(found_docs[0].relevance, calcrel_doc0);
    ASSERT_EQUAL(found_docs[1].relevance, calcrel_doc1);
    ASSERT_EQUAL(found_docs[2].relevance, calcrel_doc2);
}

// The TestSearchServer function is the entry point for running tests
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeDocumentsFromResultWithMinusWords();
    TestMatchingDocuments();
    TestSortingByRelevance();
    TestCalcAvgRating();
    TestUserPredicate();
    TestSearchExactStatus();
    TestCalcRelevance();
}