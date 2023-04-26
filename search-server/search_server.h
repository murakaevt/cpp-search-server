#pragma once

#include "log_duration.h"
#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"


#include <map>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <execution>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::sequenced_policy seq, int document_id);

    void RemoveDocument(std::execution::parallel_policy par, int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query) const;
        
    int GetDocumentCount() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::vector<int>::const_iterator begin();

    std::vector<int>::const_iterator end();

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
        int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy seq,
        std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy par,
        std::string_view raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> id_word_freqs_;                      //новая структура с ключом - id                       
    std::vector<int> document_ids_;
    std::map<std::string_view, double> res_;
    std::map<int, std::string> documents_from_request;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy par, const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        using namespace std::literals;
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    // LOG_DURATION_STREAM("Operation time", std::cout);
    auto query = ParseQuery(raw_query);
    std::sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()),
        query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());

    query.minus_words.erase(std::unique(query.minus_words.begin(),
        query.minus_words.end()), query.minus_words.end());
    auto matched_documents = SearchServer::FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            constexpr double exp = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < exp) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return SearchServer::FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    auto query = ParseQuery(raw_query);
    std::sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(std::execution::par, query.plus_words.begin(), query.plus_words.end()),
        query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(std::unique(std::execution::par, query.minus_words.begin(),
        query.minus_words.end()), query.minus_words.end());

    auto matched_documents = SearchServer::FindAllDocuments(par, query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            constexpr double exp = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < exp) {
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

    template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
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

    for (std::string_view word : query.minus_words) {
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
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy par, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(query.plus_words.size());
    
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&](std::string_view plus_word) {
        if (word_to_document_freqs_.count(plus_word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(plus_word);

        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(plus_word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
        });
    
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view minus_word) {
        if (word_to_document_freqs_.count(minus_word) == 0) {
            return;
        }
         for (const auto [document_id, _] : word_to_document_freqs_.at(minus_word)) {
             document_to_relevance.Erase(document_id);
        }
        });    
    
    std::vector<Document> matched_documents;
       
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    
    return matched_documents;
}