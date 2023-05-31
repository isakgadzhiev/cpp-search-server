#include "process_queries.h"
#include <string>
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
              [&search_server] (const std::string& query)
                        {return search_server.FindTopDocuments(query);});
    return result;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result = ProcessQueries(search_server, queries);
    size_t total_doc_count = 0;
    for (const std::vector<Document>& docs : result) {
        total_doc_count += docs.size();
    }
    std::list<Document> join_documents;
    for (const auto& docs : result) {
        join_documents.insert(join_documents.end(), docs.begin(), docs.end());
    }
    return join_documents;
}