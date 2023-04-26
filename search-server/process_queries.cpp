#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& query) {return search_server.FindTopDocuments(query);});
    return result;
}

std::vector <Document>  ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> process_queries = ProcessQueries(search_server, queries);
    std::vector<Document> result;
    for (const auto& process_query : process_queries) {
        for (const Document& doc : process_query) {
            result.insert(result.end(), doc);
        }
    }
    return result;
}